#include "MoveProcessor.hpp"
#include "logger/Logger.hpp"
#include "MoveFSM.hpp"
// Include the perception interface implementation here so the implementation
// can call methods on the perception engine without exposing OpenCV in headers.
#include "perception/IPerceptionEngine.hpp"


namespace ChessClock {


MoveProcessor::MoveProcessor(std::shared_ptr<IPerceptionEngine> perception,
                               std::unique_ptr<MoveFSM> fsm)
    : m_perceptionEngine(std::move(perception)),
      m_moveFSM(std::move(fsm)) {
        Logger::info("MoveProcessor", "Constructed MoveProcessor");
}


MoveProcessor::~MoveProcessor() {
    stop();
}

bool MoveProcessor::initialize() {
    if (!m_perceptionEngine || !m_moveFSM) {
        return false;
    }
    Logger::info("MoveProcessor", "Initializing perception engine");
    return m_perceptionEngine->initialize();
}

void MoveProcessor::start() {
    if (m_running.load()) return;

    m_perceptionEngine->start();
    m_running.store(true);
    m_thread = std::thread(&MoveProcessor::processingThreadFunc, this);
    Logger::info("MoveProcessor", "Processing thread started");
}

void MoveProcessor::stop() {
    if (!m_running.load()) return;

    m_running.store(false);
    m_perceptionEngine->stop();
    if (m_thread.joinable()) m_thread.join();
    Logger::info("MoveProcessor", "Processing thread stopped");
}

void MoveProcessor::setShutdownCallback(std::function<bool()> shouldStop) {
    m_shouldStop = std::move(shouldStop);
}

bool MoveProcessor::isRunning() const {
    return m_running.load();
}


void MoveProcessor::processingThreadFunc() {
    // Helper to check if we should continue running
    auto shouldContinue = [this]() {
        if (!m_running.load()) return false;
        if (m_shouldStop && m_shouldStop()) return false;
        return true;
    };
    
    while (shouldContinue()) {
        // 1. BLOCKING WAIT (with timeout, will wake on shutdown)
        m_perceptionEngine->waitForEvents();

        // 2. SAFETY CHECK
        if (!shouldContinue()) break;

        // 3. EVENT PROCESSING
        auto events = m_perceptionEngine->pollPerceptionEvents();
        if (!events.empty()) {
            Logger::info("MoveProcessor", "Pulled " + std::to_string(events.size()) + " events from perception");
        }

        for (const auto& event : events) {
            if (event.kind == PerceptionEvent::Kind::Move) {
                Logger::info("MoveProcessor", "Dispatching MoveEvent to FSM");
                m_moveFSM->processMoveEvent(event.move);
            } else if (event.kind == PerceptionEvent::Kind::Stability) {
                Logger::info("MoveProcessor", "Dispatching StabilityEvent to FSM");
                m_moveFSM->processStabilityEvent(event.stable);
            }
        }
    }
}

} // namespace ChessClock