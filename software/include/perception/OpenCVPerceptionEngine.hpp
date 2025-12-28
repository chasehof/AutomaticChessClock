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
            std::array<int,64> m_stabilityCounters{};
            int m_stabilityThreshold{5};
            std::atomic<bool> m_running{false};
            bool m_calibrated{false};
            bool m_boardStable{true};
            
            PerceptionState m_state{PerceptionState::UNINITIALIZED};

    };
    
} // namespace ChessClock
