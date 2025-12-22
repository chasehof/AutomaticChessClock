
#pragma once

#include <chrono>
#include <vector>

namespace ChessClock {

/**
 * Alias for a monotonic clock timepoint used for timestamps in events.
 */
using ClockTime = std::chrono::steady_clock::time_point;

/**
 * Binary occupancy state for a board square.
 */
enum class Occupancy {
    EMPTY,   /**< Square is empty */
    OCCUPIED /**< Square is occupied */
};

/**
 * Strongly-typed player color.
 */
enum class PlayerColor {
    WHITE, /**< White player */
    BLACK  /**< Black player */
};

/**
 * Enumeration of board square coordinates mapped to indices (0..63).
 * Values are laid out row-major starting from A1 = 0 to H8 = 63.
 */
enum class SquareCoordinates : int {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A2 = 8, B2, C2, D2, E2, F2, G2, H2,
    A3 = 16, B3, C3, D3, E3, F3, G3, H3,
    A4 = 24, B4, C4, D4, E4, F4, G4, H4,
    A5 = 32, B5, C5, D5, E5, F5, G5, H5,
    A6 = 40, B6, C6, D6, E6, F6, G6, H6,
    A7 = 48, B7, C7, D7, E7, F7, G7, H7,
    A8 = 56, B8, C8, D8, E8, F8, G8, H8
};

/**
 * Event emitted by the Vision thread when a square's occupancy changes.
 *
 * This event is intentionally lightweight: it records the square index,
 * the timestamp when the change was observed and the new occupancy state.
 */
struct MoveEvent {
    SquareCoordinates coordinates; /**< Square where the change occurred */
    ClockTime timestamp;           /**< Monotonic timestamp of the observation */
    Occupancy occupancy;           /**< New occupancy value for the square */
};

/**
 * Event representing a stability measurement for a square. Stability is used
 * by the FSM to confirm that transient motion has settled before confirming a move.
 */
struct MoveConfirmedEvent {
    SquareCoordinates coordinates; /**< Square being monitored for stability */
    ClockTime timestamp;           /**< Time when stability was measured */
    bool isStable;                 /**< True if the square remained stable */
};

/**
 * Snapshot of the clock state exposed to the UI or for logging.
 */
struct ClockState {
    std::chrono::milliseconds whiteTimeRemaining; /**< White player's remaining time */
    std::chrono::milliseconds blackTimeRemaining; /**< Black player's remaining time */
    PlayerColor activePlayer;                      /**< Which player is currently active */
    bool isGameOver;                               /**< True when the game is finished */
};




} // namespace ChessClock
