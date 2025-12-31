#include "perception/OpenCVPerceptionEngine.hpp"
#include <chrono>
#include "logger/Logger.hpp"
#include <sstream>
#include <cstdlib>
#include <algorithm>

// Helper: compute a simple perspective warp matrix from detected inner corners
static cv::Mat calculateWarpMatrix(const std::vector<cv::Point2f>& corners, const cv::Size& boardSize) {
    if (corners.size() < 49) return cv::Mat(); // Need all 49 for 7x7

    // The inner corners of an 800x800 board (8 squares of 100px)
    // are located at 100, 200, 300, 400, 500, 600, 700.
    std::vector<cv::Point2f> src = {
        corners.front(),                               // Top-Left inner
        corners[boardSize.width - 1],                  // Top-Right inner
        corners.back(),                                // Bottom-Right inner
        corners[(boardSize.height - 1) * boardSize.width] // Bottom-Left inner
    };

    // Correct destination: inner corners are NOT at 0 and 800!
    std::vector<cv::Point2f> dst = {
        cv::Point2f(100.f, 100.f),
        cv::Point2f(700.f, 100.f),
        cv::Point2f(700.f, 700.f),
        cv::Point2f(100.f, 700.f)
    };

    return cv::getPerspectiveTransform(src, dst);
}

namespace ChessClock {

OpenCVPerceptionEngine::OpenCVPerceptionEngine() {
    m_state = PerceptionState::UNINITIALIZED;
    Logger::info("Perception", "Perception engine constructed");
}

OpenCVPerceptionEngine::~OpenCVPerceptionEngine() {
    stop();
}

bool OpenCVPerceptionEngine::initialize() {
    // Try a set of known-good pipelines in order. The first one that opens
    // and produces frames wins. This was previously working:
    //   1640x1232 (wide FOV) -> videoscale -> 640x480 -> BGR -> appsink
    // Add a second fallback at 1280x720 in case the first fails.
    // Pipelines ordered depending on whether AC_MANUAL_EXPOSURE is set.
    // Some libcamerasrc builds block or fail when manual props are unsupported,
    // so default to auto-exposure for reliability and allow opt-in to manual.
    const bool wantManual = (std::getenv("AC_MANUAL_EXPOSURE") != nullptr);

    const std::vector<std::string> pipelines = wantManual ? std::vector<std::string>{
        "libcamerasrc exposure-mode=manual exposure-time=6000 analogue-gain=1.5 ! "
        "video/x-raw,width=1640,height=1232,framerate=30/1 ! "
        "videoscale ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false",

        "libcamerasrc exposure-mode=manual exposure-time=6000 analogue-gain=1.5 ! "
        "video/x-raw,width=1280,height=720,framerate=30/1 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false",

        // Auto-exposure fallbacks
        "libcamerasrc ! video/x-raw,width=1640,height=1232,framerate=30/1 ! "
        "videoscale ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false",

        "libcamerasrc ! video/x-raw,width=1280,height=720,framerate=30/1 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false"
    } : std::vector<std::string>{
        // Auto first for reliability
        "libcamerasrc ! video/x-raw,width=1640,height=1232,framerate=30/1 ! "
        "videoscale ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false",

        "libcamerasrc ! video/x-raw,width=1280,height=720,framerate=30/1 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false",

        // Optional manual pipelines (opt-in via env)
        "libcamerasrc exposure-mode=manual exposure-time=6000 analogue-gain=1.5 ! "
        "video/x-raw,width=1640,height=1232,framerate=30/1 ! "
        "videoscale ! video/x-raw,width=640,height=480 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false",

        "libcamerasrc exposure-mode=manual exposure-time=6000 analogue-gain=1.5 ! "
        "video/x-raw,width=1280,height=720,framerate=30/1 ! "
        "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false"
    };

    bool opened = false;
    for (size_t i = 0; i < pipelines.size(); ++i) {
        const auto &p = pipelines[i];
        Logger::info("Perception", "Opening camera via GStreamer: " + p);
        if (m_camera.open(p, cv::CAP_GSTREAMER)) {
            opened = true;
            Logger::info("Perception", "Camera opened with pipeline index " + std::to_string(i));
            break;
        }
        Logger::info("Perception", "Pipeline index " + std::to_string(i) + " failed; trying next");
    }

    if (!opened) {
        Logger::info("Perception", "GStreamer pipelines failed, trying V4L2 fallback");
        m_camera.open(0, cv::CAP_V4L2);
    }

    if (m_camera.isOpened()) {
        Logger::info("Perception", "Camera opened, backend=" + m_camera.getBackendName());
    }

    if (!m_camera.isOpened()) {
        m_state = PerceptionState::ERROR;
        Logger::info("Perception", "Failed to open camera");
        return false;
    }

    Logger::info("Perception", "Camera opened successfully on index " + std::to_string(m_deviceIndex));

    m_state = PerceptionState::UNINITIALIZED;

    // Check environment to optionally enable a debug preview window. This
    // is a development-time helper only. Enable by setting the environment
    // variable AC_ENABLE_PREVIEW when launching the binary. TODO: remove
    // before production or replace with a proper runtime flag/config.
    if (std::getenv("AC_ENABLE_PREVIEW")) {
        m_debugPreview = true;
        cv::namedWindow("PerceptionPreview", cv::WINDOW_AUTOSIZE);
        Logger::info("Perception", "Debug preview enabled (AC_ENABLE_PREVIEW)");
    }
    return true;
}

bool OpenCVPerceptionEngine::calibrate() {
    m_state = PerceptionState::CALIBRATING;
    cv::Mat frame, gray, warped;
    bool ok = m_camera.read(frame);
    static bool retriedOnce = false;
    if (!ok || frame.empty()) {
        Logger::info("Perception", "Calibration: empty/failed frame");
        if (!retriedOnce) {
            retriedOnce = true;
            Logger::info("Perception", "Reopening camera after failed frame...");
            m_camera.release();
            initialize();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return false; // retry on next calibrate call
        }
        m_state = PerceptionState::ERROR;
        return false;
    }
    
    Logger::info("Perception", "Calibration: got frame " + std::to_string(frame.cols) + "x" + std::to_string(frame.rows));

    // Live calibration preview (always on during calibration). This helps
    // diagnose framing/focus/lighting. Non-blocking: waitKey(1).
    static bool calibWindowCreated = false;
    if (!calibWindowCreated) {
        cv::namedWindow("CalibrationPreview", cv::WINDOW_AUTOSIZE);
        calibWindowCreated = true;
    }

    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::Size boardSize(7, 7);  // 7x7 inner corners = 8x8 squares
    std::vector<cv::Point2f> corners;

    // More robust corner detection: CLAHE + optional upscale + SB fallback
    cv::Mat grayClahe;
    {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(gray, grayClahe);
    }

    auto tryDetect = [&](const cv::Mat& img, double scale) -> bool {
        std::vector<cv::Point2f> tmp;
        // First, try the SB detector (more robust)
        bool okSb = cv::findChessboardCornersSB(img, boardSize, tmp,
            cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_EXHAUSTIVE | cv::CALIB_CB_ACCURACY);
        if (!okSb) {
            // Fallback to classic with adaptive threshold
            okSb = cv::findChessboardCorners(img, boardSize, tmp,
                cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
        }
        if (okSb) {
            // Scale back down if we upscaled
            for (auto &p : tmp) {
                p.x /= scale;
                p.y /= scale;
            }
            corners = std::move(tmp);
            return true;
        }
        return false;
    };

    bool found = false;
    // Try on CLAHE at native scale
    found = tryDetect(grayClahe, 1.0);
    if (!found) {
        // Try a gentle upscale to give subpixel more info
        cv::Mat up;
        const double upScale = 1.5;
        cv::resize(grayClahe, up, cv::Size(), upScale, upScale, cv::INTER_CUBIC);
        found = tryDetect(up, upScale);
    }

    if (found) {
        Logger::info("Perception", "Calibration: found " + std::to_string(corners.size()) + " corners");
        cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                         cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));
        
        m_warpMatrix = ::calculateWarpMatrix(corners, boardSize);

    cv::warpPerspective(gray, warped, m_warpMatrix, cv::Size(800, 800));
    m_refWarpedGray = warped.clone();
        int cellSize = 800 / 8;
        double boardMeanAccum = 0.0;
        for (int i = 0; i < 64; ++i) {
            int r = i / 8;
            int c = i % 8;
            cv::Rect roi(c * cellSize + 15, r * cellSize + 15, cellSize - 30, cellSize - 30);
            
            cv::Scalar mean, stddev;
            cv::meanStdDev(warped(roi), mean, stddev);
            m_refMeans[i] = mean[0];
            m_refStdDevs[i] = stddev[0];
            m_runMeans[i] = mean[0];
            m_runStdDevs[i] = std::max(stddev[0], 1.0);
            boardMeanAccum += mean[0];
        }
        m_refBoardMean = boardMeanAccum / 64.0;
        m_runBoardMean = m_refBoardMean;

        // Show a quick success preview frame with corners drawn
        cv::Mat successPreview = frame.clone();
        cv::drawChessboardCorners(successPreview, boardSize, corners, found);
        cv::putText(successPreview, "Calibration OK", cv::Point(20, 40),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 200, 0), 2);
        cv::imshow("CalibrationPreview", successPreview);
        cv::waitKey(1);

        m_calibrated = true;
        m_state = PerceptionState::RUNNING; 
        Logger::info("Perception", "Calibration successful - running");
        return true;
    }
    // Draw failure preview with hint text
    cv::Mat failPreview = frame.clone();
    cv::putText(failPreview, "Board not found", cv::Point(20, 40),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
    cv::putText(failPreview, "Need 7x7 inner corners", cv::Point(20, 75),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
    cv::imshow("CalibrationPreview", failPreview);
    cv::waitKey(1);

    m_state = PerceptionState::ERROR;
    Logger::info("Perception", "Calibration failed - board not found");
    return false;
}
void OpenCVPerceptionEngine::start() {
    if (m_running.load()) return;
    if (!m_camera.isOpened()) {
        if (!initialize()) return;
    }

    m_running.store(true);
    m_thread = std::thread(&OpenCVPerceptionEngine::perceptionThreadFunc, this);
    Logger::info("Perception", "Perception thread started");
}

void OpenCVPerceptionEngine::stop() {
    if (!m_running.load()) return;
    m_running.store(false);
    m_queueCV.notify_all();
    if (m_thread.joinable()) m_thread.join();
    if (m_debugPreview) {
        cv::destroyAllWindows();
    }
    Logger::info("Perception", "Perception thread stopped");
}

void OpenCVPerceptionEngine::setShutdownCallback(std::function<bool()> shouldStop) {
    m_shouldStop = std::move(shouldStop);
}

PerceptionState OpenCVPerceptionEngine::state() const {
    return m_state;
}

std::vector<PerceptionEvent> OpenCVPerceptionEngine::pollPerceptionEvents() {
    std::vector<PerceptionEvent> out;
    std::lock_guard lk(m_queueMutex);
    while (!m_eventQueue.empty()) {
        auto msg = m_eventQueue.front();
        m_eventQueue.pop();
        PerceptionEvent pe;
        if (msg.kind == PerceptionMessage::Kind::Move) {
            pe.kind = PerceptionEvent::Kind::Move;
            pe.move = msg.move;
        } else {
            pe.kind = PerceptionEvent::Kind::Stability;
            pe.stable = msg.stable;
        }
        out.push_back(std::move(pe));
    }
    return out;
}
bool OpenCVPerceptionEngine::isCalibrated() const {
    return m_calibrated;
}

void OpenCVPerceptionEngine::waitForEvents(){
    std::unique_lock<std::mutex> lk(m_queueMutex);
    // Wait for events OR shutdown signal (check m_running and m_shouldStop)
    m_queueCV.wait_for(lk, std::chrono::milliseconds(100), [this](){
        if (!m_running.load()) return true;  // Wake up on shutdown
        if (m_shouldStop && m_shouldStop()) return true;  // Wake up on external shutdown
        return !m_eventQueue.empty();
    });
}

void OpenCVPerceptionEngine::perceptionThreadFunc() {
    const int frameDelayMs = 33;      // Target ~30 FPS
    const int boardSizePixels = 800;  // Canonical size for 8x8 grid
    const int cellSize = boardSizePixels / 8;

    if (m_debugPreview) {
        // Ensure window is created in this thread's context
        cv::namedWindow("PerceptionPreview", cv::WINDOW_AUTOSIZE);
    }

    // 1. Initialize State
    for (auto &o : m_lastOccupancy) o = Occupancy::EMPTY;
    for (auto &o : m_prevStableBoard) o = Occupancy::EMPTY;
    m_stabilityCounters.fill(0);
    m_debugMeanThresh.fill(0.0);
    m_debugStdThresh.fill(0.0);
    m_boardStable = false;
    m_failedReads = 0;
    m_stableFrames = 0;

    // We assume m_refMeans and m_refStdDevs were populated during calibrate()
    // If not, we fall back to a hard threshold of 100.0
    
    // Helper lambda: check if we should stop (either m_running is false or
    // the external shutdown callback says to stop)
    auto shouldContinue = [this]() {
        if (!m_running.load()) return false;
        if (m_shouldStop && m_shouldStop()) return false;
        return true;
    };
    
    while (shouldContinue()) {
        // Log occasionally to prove thread is alive
        static int loopCount = 0;
        if (++loopCount % 100 == 0) {
            Logger::info("Perception", "Perception loop running (frame " + std::to_string(loopCount) + ")");
        }

        cv::Mat frame, warped, gray;
        bool ok = m_camera.read(frame);

        if (!ok || frame.empty()) {
            m_failedReads++;
            if (m_failedReads == 1 || m_failedReads % 10 == 0) {
                Logger::info("Perception", "Empty/failed frame capture (" + std::to_string(m_failedReads) + ")");
            }

            // Try one-time fallback reopen if we keep failing
            if (!m_triedFallback && m_failedReads >= 20) {
                Logger::info("Perception", "Attempting camera reopen after repeated failures");
                m_camera.release();
                m_camera.open(0, cv::CAP_V4L2);
                m_triedFallback = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMs));
            continue;
        }
        m_failedReads = 0;

        // 2. Perspective Warp (The "Flattening")
        if (!m_warpMatrix.empty()) {
            cv::warpPerspective(frame, warped, m_warpMatrix, cv::Size(boardSizePixels, boardSizePixels));
        } else {
            cv::resize(frame, warped, cv::Size(boardSizePixels, boardSizePixels));
        }

        cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0); // Light blur to preserve edges

        // Canny edge detection on the entire warped board
        // Use fixed low thresholds to detect piece edges
        cv::Mat edges;
        cv::Canny(gray, edges, 20, 60);

        // 3. 8x8 Grid Analysis
        std::vector<MoveEvent> localMoves;
        int cellIndex = 0;

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                // Calculate ROI with a margin to avoid board lines
                int x = c * cellSize;
                int y = r * cellSize;
                cv::Rect roi(x + 20, y + 20, cellSize - 40, cellSize - 40);
                
                cv::Mat sq = gray(roi);
                cv::Mat edgeRoi = edges(roi);
                
                // Count edge pixels in this square
                int edgePixels = cv::countNonZero(edgeRoi);
                int totalPixels = (cellSize - 40) * (cellSize - 40);
                double edgePercent = (100.0 * edgePixels) / totalPixels;

                // Also compute stddev for secondary signal
                cv::Scalar mean, stddev;
                cv::meanStdDev(sq, mean, stddev);

                // CANNY EDGE-BASED DETECTION
                // Pieces are 3D objects with distinct silhouettes
                // Empty squares are flat with minimal edges
                Occupancy occ;

                if (m_calibrated) {
                    // Primary signal: edge pixel density
                    // Very low thresholds since edge density is sparse
                    bool hasStrongEdges = (edgePercent > 3.0);   // definitely a piece
                    bool hasModerateEdges = (edgePercent > 1.5); // likely a piece
                    
                    // Secondary signal: texture (stddev) confirms 3D object
                    bool hasTexture = (stddev[0] > 12.0);
                    
                    // DECISION: Edge-centric logic
                    bool occupiedNow = hasStrongEdges || (hasModerateEdges && hasTexture);

                    // Hysteresis: require edge to drop significantly to clear
                    bool clearSignals = (edgePercent < 1.0) && (stddev[0] < 8.0);

                    if (m_lastOccupancy[cellIndex] == Occupancy::OCCUPIED) {
                        occ = clearSignals ? Occupancy::EMPTY : Occupancy::OCCUPIED;
                    } else {
                        occ = occupiedNow ? Occupancy::OCCUPIED : Occupancy::EMPTY;
                    }
                } else {
                    occ = (mean[0] < 100.0) ? Occupancy::OCCUPIED : Occupancy::EMPTY;
                }
                
                // Store debug metrics
                m_debugDiffs[cellIndex] = edgePercent;
                m_debugRefDiff[cellIndex] = edgePercent;
                m_debugEdge[cellIndex] = edgePercent;
                m_debugZNCC[cellIndex] = 0.0;
                m_debugSquareStdDevs[cellIndex] = stddev[0];
                m_debugMeanThresh[cellIndex] = 6.0;   // moderate edge threshold
                m_debugStdThresh[cellIndex] = 10.0;   // strong edge threshold

                // 4. Per-Square Stability Check
                if (occ != m_lastOccupancy[cellIndex]) {
                    MoveEvent me;
                    me.coordinates = static_cast<SquareCoordinates>(cellIndex);
                    me.timestamp = std::chrono::steady_clock::now();
                    me.occupancy = occ;
                    
                    localMoves.push_back(me);
                    m_stabilityCounters[cellIndex] = 0; // Reset stability on change
                        Logger::info("Perception", "Detected move on cell " + std::to_string(cellIndex));
                } else {
                    if (m_stabilityCounters[cellIndex] < m_stabilityThreshold) {
                        m_stabilityCounters[cellIndex]++;
                    }
                }

                m_lastOccupancy[cellIndex] = occ;
                cellIndex++;
            }
        }

        // 5. Global Board Stability Logic
        // The board is only stable if EVERY square has been unchanged for N frames
        bool allSquaresStable = true;
        for (int count : m_stabilityCounters) {
            if (count < m_stabilityThreshold) {
                allSquaresStable = false;
                break;
            }
        }

        if (allSquaresStable) {
            m_stableFrames++;
            // Slow rebaseline when stable: blend running means/stddev toward current
            if (m_stableFrames > 5) {
                const double alpha = 0.05; // slow blend
                for (int i = 0; i < 64; ++i) {
                    int r = i / 8;
                    int c = i % 8;
                    int x = c * cellSize;
                    int y = r * cellSize;
                    cv::Rect roi(x + 15, y + 15, cellSize - 30, cellSize - 30);
                    cv::Scalar m, s;
                    cv::meanStdDev(gray(roi), m, s);
                    m_runMeans[i] = m_runMeans[i] * (1.0 - alpha) + m[0] * alpha;
                    m_runStdDevs[i] = m_runStdDevs[i] * (1.0 - alpha) + s[0] * alpha;
                }
                double bm = cv::mean(gray)[0];
                m_runBoardMean = m_runBoardMean * (1.0 - alpha) + bm * alpha;
            }
        } else {
            m_stableFrames = 0;
        }

        std::vector<StabilityEvent> localStables;
        
        // Transition: Moving -> Stable
        if (allSquaresStable && !m_boardStable) {
            StabilityEvent se;
            se.timestamp = std::chrono::steady_clock::now();
            se.isStable = true;
            se.prev = m_prevStableBoard;
            se.curr = m_lastOccupancy;
            localStables.push_back(se);
                Logger::info("Perception", "Board became stable; pushing StabilityEvent");
            
            m_prevStableBoard = m_lastOccupancy; // Snapshot the new "Ground Truth"
            m_boardStable = true;
        } 
        // Transition: Stable -> Moving (Hand entered the frame)
        else if (!allSquaresStable && m_boardStable) {
            m_boardStable = false;
            StabilityEvent se;
            se.isStable = false;
            se.timestamp = std::chrono::steady_clock::now();
            localStables.push_back(se);
                Logger::info("Perception", "Board became unstable");
        }

        // 6. Push to Queue 1 (The Bridge to Thread 2)
        if (!localMoves.empty() || !localStables.empty()) {
            std::lock_guard lk(m_queueMutex);
            for (auto &mv : localMoves) {
                m_eventQueue.push({PerceptionMessage::Kind::Move, mv, {}});
                    Logger::info("Perception", "Queued MoveEvent");
            }
            for (auto &st : localStables) {
                m_eventQueue.push({PerceptionMessage::Kind::Stability, {}, st});
                    Logger::info("Perception", "Queued StabilityEvent");
            }
            m_queueCV.notify_all();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMs));
    }
}

void OpenCVPerceptionEngine::runPreviewLoop() {
    // Simple preview loop that runs in main thread
    // Shows the warped board with occupancy overlay
    // Press 'q' or Enter to exit
    
    if (!m_camera.isOpened()) {
        Logger::info("Perception", "runPreviewLoop: camera not open");
        return;
    }
    if (!m_calibrated) {
        Logger::info("Perception", "runPreviewLoop: not calibrated");
        return;
    }
    
    Logger::info("Perception", "Starting preview loop - press 'q' or Enter to start game");
    Logger::info("Perception", "Warp matrix size: " + std::to_string(m_warpMatrix.rows) + "x" + std::to_string(m_warpMatrix.cols));
    Logger::info("Perception", "Warp matrix empty: " + std::string(m_warpMatrix.empty() ? "YES" : "NO"));
    
    cv::namedWindow("Preview", cv::WINDOW_NORMAL);
    cv::resizeWindow("Preview", 620, 620);  // Single occupancy view
    
    const int cellSize = 100;  // 800 / 8
    
    while (true) {
        // Check shutdown
        if (m_shouldStop && m_shouldStop()) break;
        
        cv::Mat frame;
        if (!m_camera.read(frame) || frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            continue;
        }
        
        // Log frame size once
        static bool loggedSize = false;
        if (!loggedSize) {
            Logger::info("Perception", "Frame size: " + std::to_string(frame.cols) + "x" + std::to_string(frame.rows));
            loggedSize = true;
        }
        
        // Warp to flat 800x800 board using calibration matrix
        cv::Mat warped, gray;
        if (!m_warpMatrix.empty()) {
            cv::warpPerspective(frame, warped, m_warpMatrix, cv::Size(800, 800));
        } else {
            // No warp matrix - just resize
            cv::resize(frame, warped, cv::Size(800, 800));
            cv::putText(warped, "NO WARP MATRIX!", cv::Point(100, 400), 
                       cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 255), 3);
        }
        
    cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);
    // Light blur to reduce noise but preserve edges
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);

    // Canny edge detection on the entire warped board
    // Use fixed low thresholds to detect piece edges
    // Pieces have strong silhouette edges that will survive
    cv::Mat edges;
    cv::Canny(gray, edges, 20, 60);  // Low thresholds to catch piece edges
    
    // Debug: count total edge pixels to verify Canny is working
    int totalEdges = cv::countNonZero(edges);
    static bool loggedEdges = false;
    if (!loggedEdges) {
        Logger::info("Perception", "Total edge pixels: " + std::to_string(totalEdges));
        loggedEdges = true;
    }
        
    // Create color display from warped frame
    cv::Mat warpedDisplay = warped.clone();

        // Use same enhanced logic as perception thread for preview overlay
        // No hysteresis for preview - just show instantaneous detection
        std::array<Occupancy,64> previewOcc;
        previewOcc.fill(Occupancy::EMPTY);

        for (int idx = 0; idx < 64; ++idx) {
            int r = idx / 8;
            int c = idx % 8;
            int x = c * cellSize + 20;
            int y = r * cellSize + 20;
            int w = cellSize - 40;
            int h = cellSize - 40;
            cv::Rect roi(x, y, w, h);
            
            cv::Mat sq = gray(roi);
            cv::Mat edgeRoi = edges(roi);

            // Count edge pixels in this square
            int edgePixels = cv::countNonZero(edgeRoi);
            int totalPixels = w * h;
            double edgePercent = (100.0 * edgePixels) / totalPixels;

            // Also compute stddev for secondary signal
            cv::Scalar mean, stddev;
            cv::meanStdDev(sq, mean, stddev);

            // CANNY EDGE-BASED DETECTION
            // Pieces are 3D objects with distinct silhouettes and surface details
            // Empty squares are flat with minimal edges (just board texture/grain)
            //
            // Very low thresholds since edge density is sparse:
            // - Empty squares: typically < 1% edge pixels
            // - Pieces: typically > 2-3% edge pixels
            
            bool hasStrongEdges = (edgePercent > 3.0);   // definitely a piece
            bool hasModerateEdges = (edgePercent > 1.5); // likely a piece
            bool hasTexture = (stddev[0] > 12.0);        // 3D object adds texture
            
            // DECISION: Edge pixel density is the primary signal
            // - Strong edges = occupied
            // - Moderate edges + texture confirmation = occupied
            bool occupiedNow = hasStrongEdges || (hasModerateEdges && hasTexture);

            // Simple instantaneous detection for preview (no hysteresis)
            Occupancy occ = occupiedNow ? Occupancy::OCCUPIED : Occupancy::EMPTY;
            previewOcc[idx] = occ;

            // Draw semi-transparent overlay
            cv::Scalar color = (occ == Occupancy::OCCUPIED) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
            cv::Mat roiMat = warpedDisplay(roi);
            cv::Mat colorOverlay(h, w, CV_8UC3, color);
            cv::addWeighted(colorOverlay, 0.3, roiMat, 0.7, 0, roiMat);
            
            // Draw border
            cv::rectangle(warpedDisplay, roi, cv::Scalar(255, 255, 255), 1);
            
            // Draw text: edge% (primary signal), stddev (secondary)
            cv::putText(warpedDisplay, std::to_string(idx), cv::Point(x + 2, y + 12), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(255, 255, 0), 1);
            
            // Edge percent - the key metric
            std::ostringstream epct;
            epct << "e:" << std::fixed << std::setprecision(1) << edgePercent << "%";
            cv::putText(warpedDisplay, epct.str(), cv::Point(x + 2, y + 26), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(255, 255, 255), 1);
            
            // Stddev
            cv::putText(warpedDisplay, "s:" + std::to_string((int)stddev[0]), cv::Point(x + 2, y + 40), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(200, 200, 255), 1);
            
            // Show which signals are hitting
            std::string hits = "";
            if (hasStrongEdges) hits += "E";
            else if (hasModerateEdges) hits += "e";
            if (hasTexture) hits += "t";
            cv::putText(warpedDisplay, hits.empty() ? "-" : hits, cv::Point(x + 2, y + 54), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(0, 255, 255), 1);
        }
        
        // Scale warped display to 600x600 and show alone
        cv::Mat warpedSmall;
        cv::resize(warpedDisplay, warpedSmall, cv::Size(600, 600));
        cv::putText(warpedSmall, "WARPED + OCCUPANCY", cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
        cv::putText(warpedSmall, "Press 'q' to start", cv::Point(10, 580), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

        cv::imshow("Preview", warpedSmall);
        int key = cv::waitKey(33);
        if (key == 'q' || key == 'Q' || key == 13 || key == 10) {
            break;
        }
    }
    
    cv::destroyWindow("Preview");
    Logger::info("Perception", "Preview loop ended");;
}

} // namespace ChessClock