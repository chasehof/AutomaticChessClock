#pragma once

#include "types/ChessEvents.hpp"
#include <chrono>
#include <atomic>
#include <array>

namespace ChessClock {

/**
 * Clock engine responsible for tracking both players' remaining time,
 * applying increments, pausing/starting the clock and emitting state
 * snapshots for the UI.
 */
class ClockEngine {
public:
    /**
     * Configuration used to initialize the clock engine.
     */
    struct Config {
        std::chrono::milliseconds initialTime;    /**< Starting time per player */
        std::chrono::milliseconds incrementPerMove; /**< Increment applied after each move */
    };

    /**
     * Construct the engine with the provided configuration.
     *
     * @param config Configuration (initial time and increment).
     */
    explicit ClockEngine(const Config& config);

    /**
     * Start or resume the running clock.
     */
    void start();

    /**
     * Pause the running clock.
     */
    void pause();

    /**
     * Update internal timers; should be called periodically (e.g., from the
     * clock thread) to advance time based on a monotonic clock.
     */
    void update();

    /**
     * Notify the engine that a move has been confirmed. The engine will
     * apply increments to the appropriate player and may toggle the active player.
     *
     * @param event MoveConfirmedEvent payload (source/destination/details).
     */
    void onMoveConfirmed(const MoveConfirmedEvent& event);

    /**
     * Capture a snapshot of the current clock state for UI/logging.
     *
     * @return Current ClockState (times, active player, finished flag).
     */
    ClockState getState() const;

private:
    Config m_config; /**< Configuration copy */

    std::array<std::chrono::milliseconds, 2> m_timeRemaining; /**< [WHITE, BLACK] */

    PlayerColor m_activePlayer{ PlayerColor::WHITE }; /**< Currently active player */

    std::atomic<bool> m_isRunning{ false }; /**< Atomic running flag for thread-safety */

    std::chrono::steady_clock::time_point m_lastUpdateTime; /**< Last time update() ran */

    /**
     * Apply the configured increment to the given player.
     */
    void applyIncrement(PlayerColor player);
};

} // namespace ChessClock