#pragma once

#include <opencv2/opencv.hpp>
#include "perception/IPerceptionEngine.hpp"
#include "types/ChessEvents.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <queue>
#include <array>
#include <string>
#include <functional>


namespace ChessClock
{

    class OpenCVPerceptionEngine : public IPerceptionEngine{

        public:
            OpenCVPerceptionEngine();
            ~OpenCVPerceptionEngine() override;

            bool initialize() override;
            bool calibrate() override;

            void start() override;
            void stop() override;

            PerceptionState state() const override;

            std::vector<PerceptionEvent> pollPerceptionEvents() override;

            bool isCalibrated() const override;

            void waitForEvents();
            
            // Allow external shutdown signal (e.g., from SIGINT handler)
            void setShutdownCallback(std::function<bool()> shouldStop);
            
            // Simple preview loop - call from main thread to show occupancy grid
            // Returns when user presses 'q' or 'Enter', or shutdown is signaled
            void runPreviewLoop();

        private:
            void perceptionThreadFunc();

            // Single, ordered queue for all perception messages. This preserves
            // temporal ordering between Move and Stability notifications and
            // simplifies consumer logic.
            struct PerceptionMessage {
                enum class Kind { Move, Stability } kind;
                MoveEvent move;           // valid when kind==Move
                StabilityEvent stable;    // valid when kind==Stability
            };

            std::queue<PerceptionMessage> m_eventQueue;

            std::mutex m_queueMutex;
            std::condition_variable m_queueCV;

            cv::VideoCapture m_camera;
            cv::Mat m_warpMatrix;
            std::thread m_thread;
            std::array<Occupancy,64> m_lastOccupancy{};
            std::array<Occupancy,64> m_prevStableBoard{};
            std::array<double, 64> m_refMeans{};
            std::array<double, 64> m_refStdDevs{};
            std::array<double, 64> m_runMeans{};
            std::array<double, 64> m_runStdDevs{};
            double m_refBoardMean{0.0};
            double m_runBoardMean{0.0};
            // Debug metrics for visualization
            std::array<double, 64> m_debugDiffs{};
            std::array<double, 64> m_debugSquareStdDevs{};
            std::array<double, 64> m_debugMeanThresh{};
            std::array<double, 64> m_debugStdThresh{};
            std::array<double, 64> m_debugRefDiff{};
            std::array<double, 64> m_debugEdge{};
            std::array<double, 64> m_debugZNCC{};
            // Hysteresis and stability
            int m_stableFrames{0};
            
            std::array<int,64> m_stabilityCounters{};
            int m_stabilityThreshold{5};
            int m_deviceIndex{-1};
            std::atomic<bool> m_running{false};
            bool m_calibrated{false};
            bool m_boardStable{true};
            int m_failedReads{0};
            bool m_triedFallback{false};
            bool m_triedPipeline{false};
            std::string m_pipeline;
            // Stored empty-board reference (warped, grayscale) for absdiff/SSIM-style occupancy tests
            cv::Mat m_refWarpedGray;
            // Illumination normalization buffers
            cv::Mat m_lastIllum;
            
            PerceptionState m_state{PerceptionState::UNINITIALIZED};
            // Debug preview flag: when true, the perception thread will show
            // an OpenCV imshow window with the warped board for development.
            // TODO: remove this debug helper before production / make it a
            // proper runtime option behind a config system.
            bool m_debugPreview{false};
            bool m_calibrationPreviewCreated{false};
            
            // Optional external shutdown callback (for SIGINT handling)
            std::function<bool()> m_shouldStop;
    };
}    // namespace ChessClock
