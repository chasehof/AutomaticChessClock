#pragma once

#include "types/ChessEvents.hpp"
#include <vector>
#include <optional>
#include <functional>

/**
 * Move finite-state machine (FSM) responsible for interpreting low-level
 * vision events (occupancy changes and stability notifications) and
 * confirming completed moves.
 *
 * The FSM owns a small internal state machine with the states described in
 * the project's architecture doc. When a move is confirmed the FSM invokes
 * the provided callback with a MoveConfirmedEvent.
 */
class MoveFSM {
public:
    /**
     * FSM state enumeration.
     */
    enum class State {
        IDLE,          /**< Board stable; waiting for a piece to be lifted */
        PIECE_LIFTED,  /**< A piece was removed from a square */
        PIECE_PLACED,  /**< A piece was placed on a square */
        CONFIRMED_MOVE /**< A move has been confirmed */
    };

    /**
     * Callback invoked when the FSM confirms a move. The callback receives
     * a MoveConfirmedEvent object (defined elsewhere in the codebase).
     */
    using MoveConfirmedCallback = std::function<void(const MoveConfirmedEvent&)>;

    /**
     * Construct the FSM with a move-confirmed callback.
     *
     * @param callback Function to call when a move is confirmed.
     */
    explicit MoveFSM(MoveConfirmedCallback callback);

    /**
     * Process a raw occupancy change event emitted by the Vision thread.
     * This drives the FSM transitions.
     *
     * @param event MoveEvent describing square index, occupancy and timestamp.
     */
    void processMoveEvent(const MoveEvent& event);

    /**
     * Process a stability measurement for a square. Stability events are
     * used to filter transient motion and to confirm placement.
     *
     * @param event StabilityEvent describing the square and stability flag.
     */
    void processStabilityEvent(const StabilityEvent& event);

    /**
     * Reset the FSM to its initial state (IDLE) and clear any pending state.
     */
    void reset();

    /**
     * Query the current FSM state.
     *
     * @return Current State value.
     */
    State getCurrentState() const;

private:
    State m_currentState{State::IDLE};               /**< Current FSM state */
    MoveConfirmedCallback m_moveConfirmedCallback;   /**< Callback for confirmed moves */

    std::optional<SquareCoordinates> m_sourceSquare;      /**< Detected source square */
    std::optional<SquareCoordinates> m_destinationSquare; /**< Detected destination square */

    /**
     * Handle an incoming MoveEvent while in the IDLE state.
     */
    void handleIdleState(const MoveEvent& event);

    /**
     * Handle an incoming MoveEvent while in the PIECE_LIFTED state.
     */
    void handlePieceLiftedState(const MoveEvent& event);

    /**
     * Handle an incoming MoveEvent while in the PIECE_PLACED state.
     */
    void handlePiecePlacedState(const MoveEvent& event);

    /**
     * Return true if the currently observed transition is an adjustment
     * (piece lifted and replaced on the same square) rather than a move.
     */
    bool isAdjustment() const;

    /**
     * Internal helper to transition to a new state and perform any
     * associated side-effects (clearing fields, etc.).
     */
    void transitionToState(State newState);
};