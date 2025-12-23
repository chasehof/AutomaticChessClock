#include "ClockEngine.hpp"


namespace ChessClock {

ClockEngine::ClockEngine(const Config& config)
    : m_initialTime(config.initialTime),
      m_incrementPerMove(config.incrementPerMove),
      m_config(config),

{
    m_timeRemaining[static_cast<int>(PlayerColor::WHITE)] = m_initialTime;
    m_timeRemaining[static_cast<int>(PlayerColor::BLACK)] = m_initialTime;
   
}

void ClockEngine::start() {
    if (!m_isRunning) {
        m_isRunning = true;
        m_lastUpdateTime = std::chrono::steady_clock::now();
    }
}

void ClockEngine::pause() {
    if (m_isRunning) {
        update(); // Update time before pausing
        m_isRunning = false;
    }
}


void ClockEngine::update() {
    if (!m_isRunning) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdateTime);
    m_timeRemaining[static_cast<int>(m_activePlayer)] -= elapsed;
    m_lastUpdateTime = now;

}


void ClockEngine::onMoveConfirmed(const MoveConfirmedEvent& event) {
    // Apply increment to the player who just moved
    applyIncrement(m_activePlayer);

    // Switch active player
    m_activePlayer = (m_activePlayer == PlayerColor::WHITE) ? PlayerColor::BLACK : PlayerColor::WHITE;

    // Reset last update time to now for accurate timing
    m_lastUpdateTime = std::chrono::steady_clock::now();
}


ClockState ClockEngine::getState() const {
    return ClockState{
        .whiteTimeRemaining = m_timeRemaining[static_cast<int>(PlayerColor::WHITE)],
        .blackTimeRemaining = m_timeRemaining[static_cast<int>(PlayerColor::BLACK)],
        .activePlayer = m_activePlayer,
        .isGameOver = m_timeRemaining[static_cast<int>(PlayerColor::WHITE)] <= std::chrono::milliseconds(0) ||
                        m_timeRemaining[static_cast<int>(PlayerColor::BLACK)] <= std::chrono::milliseconds(0)
    };
}


void ClockEngine::applyIncrement(PlayerColor player) {
    m_timeRemaining[static_cast<int>(player)] += m_config.incrementPerMove;

}



} // namespace ChessClock