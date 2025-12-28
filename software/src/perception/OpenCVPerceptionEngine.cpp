#include "perception/OpenCVPerceptionEngine.hpp"
#include <chrono>

// Helper: compute a simple perspective warp matrix from detected inner corners
static cv::Mat calculateWarpMatrix(const std::vector<cv::Point2f>& corners, const cv::Size& boardSize) {
    // corners are inner chessboard corners arranged row-major
    if (corners.size() < 4) return cv::Mat();

    // pick the four extreme inner corners
    cv::Point2f tl = corners.front();
    cv::Point2f tr = corners[boardSize.width - 1];
    cv::Point2f bl = corners[(boardSize.height - 1) * boardSize.width];
    cv::Point2f br = corners.back();

    // map them to a canonical square of size 800x800
    std::vector<cv::Point2f> src{tl, tr, br, bl};
    std::vector<cv::Point2f> dst{
        cv::Point2f(0.f, 0.f),
        cv::Point2f(800.f, 0.f),
        cv::Point2f(800.f, 800.f),
        cv::Point2f(0.f, 800.f)
    };

    return cv::getPerspectiveTransform(src, dst);
}

namespace ChessClock {

OpenCVPerceptionEngine::OpenCVPerceptionEngine() {
    m_state = PerceptionState::UNINITIALIZED;
}

OpenCVPerceptionEngine::~OpenCVPerceptionEngine() {
    stop();
}

bool OpenCVPerceptionEngine::initialize() {
    // Try to open the default camera
    if (!m_camera.isOpened()) {
        m_camera.open(0);
    }

    if (!m_camera.isOpened()) {
        m_state = PerceptionState::ERROR;
        return false;
    }

    m_state = PerceptionState::UNINITIALIZED;
    return true;
}

bool OpenCVPerceptionEngine::calibrate() {
    m_state = PerceptionState::CALIBRATING;
    cv::Mat frame, gray;
    m_camera >> frame;
    if (frame.empty()) {
        m_state = PerceptionState::ERROR;
        return false;
    }
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    cv::Size boardSize(7, 7); // Number of inner corners per chessboard row and column
    std::vector<cv::Point2f> corners;

    bool found = cv::findChessboardCorners(gray, boardSize, corners,
                                           cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);

    if (found) {
        cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                         cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));
        m_warpMatrix = ::calculateWarpMatrix(corners, boardSize);

        m_calibrated = true;
        m_state = PerceptionState::RUNNING;

        return true;
    }
    m_state = PerceptionState::ERROR;
    return false;
}

void OpenCVPerceptionEngine::start() {
    if (m_running.load()) return;
    if (!m_camera.isOpened()) {
        if (!initialize()) return;
    }

    m_running.store(true);
    m_thread = std::thread(&OpenCVPerceptionEngine::perceptionThreadFunc, this);
}

void OpenCVPerceptionEngine::stop() {
    if (!m_running.load()) return;
    m_running.store(false);
    m_queueCV.notify_all();
    if (m_thread.joinable()) m_thread.join();
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

void OpenCVPerceptionEngine::perceptionThreadFunc() {
    const int frameDelayMs = 100;
    const int boardSizePixels = 800; // per calculateWarpMatrix destination

    // Initialize last occupancy to EMPTY
    for (auto &o : m_lastOccupancy) o = Occupancy::EMPTY;
    // Initialize previous stable board snapshot to EMPTY as well
    for (auto &o : m_prevStableBoard) o = Occupancy::EMPTY;

    while (m_running.load()) {
        cv::Mat frame;
        m_camera >> frame;
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMs));
            continue;
        }

        cv::Mat warped;
        if (!m_warpMatrix.empty()) {
            cv::warpPerspective(frame, warped, m_warpMatrix, cv::Size(boardSizePixels, boardSizePixels));
        } else {
            // Fallback: resize to canonical
            cv::resize(frame, warped, cv::Size(boardSizePixels, boardSizePixels));
        }

        cv::Mat gray;
        cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);

        // Analyze 8x8 grid and produce move events; compute board-wide stability
        int cell = 0;
        int cellSize = boardSizePixels / 8;
        std::vector<MoveEvent> localMoves;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int x = c * cellSize;
                int y = r * cellSize;
                cv::Rect roi(x + cellSize/8, y + cellSize/8, cellSize*3/4, cellSize*3/4);
                cv::Mat sq = gray(roi);
                double mean = cv::mean(sq)[0];
                // simple threshold: darker => occupied (piece)
                Occupancy occ = (mean < 100.0) ? Occupancy::OCCUPIED : Occupancy::EMPTY;

                if (occ != m_lastOccupancy[cell]) {
                    MoveEvent me;
                    me.coordinates = static_cast<SquareCoordinates>(cell);
                    me.timestamp = std::chrono::steady_clock::now();
                    me.occupancy = occ;
                    localMoves.push_back(me);
                    m_stabilityCounters[cell] = 0;
                } else {
                    // increment stability counter
                    m_stabilityCounters[cell] += 1;
                }

                m_lastOccupancy[cell] = occ;
                ++cell;
            }
        }

        // If we observed any occupancy changes this frame, the board is not stable
        if (!localMoves.empty()) {
            m_boardStable = false;
        }

        // Determine if entire board has been stable for required threshold
        bool boardNowStable = true;
        for (int i = 0; i < 64; ++i) {
            if (m_stabilityCounters[i] < m_stabilityThreshold) { boardNowStable = false; break; }
        }

        std::vector<StabilityEvent> localStables;
        if (boardNowStable && !m_boardStable) {
            // Board transitioned to stable — emit a single stability event with
            // both previous and current stable board snapshots so consumers
            // don't need to maintain duplicate board state.
            StabilityEvent se;
            se.timestamp = std::chrono::steady_clock::now();
            se.isStable = true;
            se.prev = m_prevStableBoard;
            se.curr = m_lastOccupancy;
            localStables.push_back(se);
            // update previous-stable to the current snapshot for next time
            m_prevStableBoard = se.curr;
            m_boardStable = true;
        }

        if (!localMoves.empty() || !localStables.empty()) {
            std::lock_guard lk(m_queueMutex);
            for (auto &mv : localMoves) {
                OpenCVPerceptionEngine::PerceptionMessage pm{};
                pm.kind = OpenCVPerceptionEngine::PerceptionMessage::Kind::Move;
                pm.move = mv;
                m_eventQueue.push(pm);
            }
            for (auto &st : localStables) {
                OpenCVPerceptionEngine::PerceptionMessage pm{};
                pm.kind = OpenCVPerceptionEngine::PerceptionMessage::Kind::Stability;
                pm.stable = st;
                m_eventQueue.push(pm);
            }
            m_queueCV.notify_all();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMs));
    }
}


} // namespace ChessClock

