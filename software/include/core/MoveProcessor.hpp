#pragma once

#include <core/IMoveProcessor.hpp>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

namespace ChessClock {

class MoveFSM; // forward
class IPerceptionEngine; // forward

class MoveProcessor : public IMoveProcessor {
public:
    MoveProcessor(std::shared_ptr<IPerceptionEngine> perception, std::unique_ptr<MoveFSM> fsm);
    ~MoveProcessor() override;

    bool initialize();
    void start();
    void stop();
    bool isRunning() const;
    
    // Allow external shutdown signal (e.g., from SIGINT handler)
    void setShutdownCallback(std::function<bool()> shouldStop);

private:
    void processingThreadFunc();

    std::shared_ptr<IPerceptionEngine> m_perceptionEngine;
    std::unique_ptr<MoveFSM> m_moveFSM;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::function<bool()> m_shouldStop;
};

} // namespace ChessClock