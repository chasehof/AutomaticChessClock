#include "MoveFSM.hpp"
#include <array>
#include <optional>
#include <algorithm>

namespace ChessClock {

MoveFSM::MoveFSM(MoveConfirmedCallback callback)
        : m_moveConfirmedCallback(std::move(callback)),
            m_currentState(MoveFSM::State::IDLE) {
}


void MoveFSM::processMoveEvent(const MoveEvent& event) {
    switch (m_currentState) {
        case MoveFSM::State::IDLE:
            handleIdleState(event);
            break;
        case MoveFSM::State::PIECE_LIFTED:
            handlePieceLiftedState(event);
            break;
        case MoveFSM::State::PIECE_PLACED:
            handlePiecePlacedState(event);
            break;
    }
}

void MoveFSM::processStabilityEvent(const StabilityEvent& event) {
    if (!event.isStable || m_currentState != MoveFSM::State::PIECE_PLACED) { return; }

    if (isAdjustment()) {
        reset();
        return;
    }

    // Use the stable snapshots provided by the perception engine. The
    // StabilityEvent contains both the previous and current stable board
    // snapshots so the FSM doesn't need to keep its own copy.
    const auto &prev = event.prev;
    const auto &curr = event.curr;

    // Attempt to classify the board diff into a concrete move.
    auto maybeMove = classifyBoardDiff(prev, curr);
    if (maybeMove.has_value()) {
        // Build the MoveConfirmedEvent here and emit callback immediately
        MoveConfirmedEvent ev{};
        ev.source = maybeMove->source;
        ev.destination = maybeMove->destination;
        ev.timestamp = std::chrono::steady_clock::now();

        m_moveConfirmedCallback(ev);

        // Transition to confirmed; perception already provided the stable
        // snapshot so we don't store it locally. transition will perform
        // reset which clears temporary sets.
        transitionToState(MoveFSM::State::CONFIRMED_MOVE);
        return;
    }

    // No confident classification; reset FSM and leave lastStableBoard unchanged.
    reset();
}

void MoveFSM::reset() {
    m_currentState = MoveFSM::State::IDLE;
    m_sourceSquare.reset();
    m_destinationSquare.reset();
    m_emptiedSquares.clear();
    m_occupiedSquares.clear();

}

MoveFSM::State MoveFSM::getCurrentState() const {
    return m_currentState;

}


void MoveFSM::handleIdleState(const MoveEvent& event) {
    if (event.occupancy == Occupancy::EMPTY) {
        // A piece was lifted
        m_emptiedSquares.insert(event.coordinates);
        m_sourceSquare = event.coordinates;
    transitionToState(MoveFSM::State::PIECE_LIFTED);
    }
}

void MoveFSM::handlePieceLiftedState(const MoveEvent& event) {
    if (event.occupancy == Occupancy::OCCUPIED) {
        // A piece was placed
        m_occupiedSquares.insert(event.coordinates);
        m_destinationSquare = event.coordinates;
    transitionToState(MoveFSM::State::PIECE_PLACED);
    } else {
        // Another piece was lifted; update source square
        m_emptiedSquares.insert(event.coordinates);
    }
}

void MoveFSM::handlePiecePlacedState(const MoveEvent& event) {
    if (event.occupancy == Occupancy::OCCUPIED) {
        // Another piece was placed; update destination square
        m_occupiedSquares.insert(event.coordinates);
        m_destinationSquare = event.coordinates;
    } else {
        // A piece was lifted again; reset FSM
        reset();
    }
    
}

bool MoveFSM::isAdjustment() const {

    return m_sourceSquare.has_value() && m_destinationSquare.has_value() &&
           m_sourceSquare.value() == m_destinationSquare.value();
}

void MoveFSM::transitionToState(MoveFSM::State newState) {
    m_currentState = newState;
    if (newState == MoveFSM::State::IDLE) { reset(); }

    if (newState == MoveFSM::State::CONFIRMED_MOVE) {
        // Reset state; MoveConfirmedEvent is emitted by the stability path
        // which calls the callback prior to transitioning here.
        reset();
    }
}

// Helper: compute diff and try to classify special moves. Conservative — if
// ambiguous, return std::nullopt.
std::optional<MoveFSM::MoveCandidate> MoveFSM::classifyBoardDiff(const std::array<Occupancy,64>& prev,
                                                                  const std::array<Occupancy,64>& curr) const {
    std::vector<SquareCoordinates> emptied;
    std::vector<SquareCoordinates> occupied;
    for (int i = 0; i < 64; ++i) {
        if (prev[i] == Occupancy::OCCUPIED && curr[i] == Occupancy::EMPTY) {
            emptied.push_back(static_cast<SquareCoordinates>(i));
        } else if (prev[i] == Occupancy::EMPTY && curr[i] == Occupancy::OCCUPIED) {
            occupied.push_back(static_cast<SquareCoordinates>(i));
        }
    }

    // Normal single move -> return candidate only
    if (emptied.size() == 1 && occupied.size() == 1) {
        return MoveCandidate{ emptied.front(), occupied.front() };
    }

    // Castling detection (two squares emptied, two squares occupied)
    if (emptied.size() == 2 && occupied.size() == 2) {
        auto contains = [](const std::vector<SquareCoordinates>& v, int idx){
            return std::find(v.begin(), v.end(), static_cast<SquareCoordinates>(idx)) != v.end();
        };

        // White king-side: E1(4), H1(7) emptied -> G1(6), F1(5) occupied
        if (contains(emptied, 4) && contains(emptied, 7) && contains(occupied, 6) && contains(occupied, 5)) {
            return MoveCandidate{ static_cast<SquareCoordinates>(4), static_cast<SquareCoordinates>(6) };
        }
        // White queen-side: E1(4), A1(0) emptied -> C1(2), D1(3) occupied
        if (contains(emptied, 4) && contains(emptied, 0) && contains(occupied, 2) && contains(occupied, 3)) {
            return MoveCandidate{ static_cast<SquareCoordinates>(4), static_cast<SquareCoordinates>(2) };
        }

        // Mirror patterns for black (rank 8)
        // Black king-side: E8(60), H8(63) emptied -> G8(62), F8(61) occupied
        if (contains(emptied, 60) && contains(emptied, 63) && contains(occupied, 62) && contains(occupied, 61)) {
            return MoveCandidate{ static_cast<SquareCoordinates>(60), static_cast<SquareCoordinates>(62) };
        }
        // Black queen-side: E8(60), A8(56) emptied -> C8(58), D8(59) occupied
        if (contains(emptied, 60) && contains(emptied, 56) && contains(occupied, 58) && contains(occupied, 59)) {
            return MoveCandidate{ static_cast<SquareCoordinates>(60), static_cast<SquareCoordinates>(58) };
        }
    }

    // Capture / en-passant heuristic: occupied==1 and emptied>=1
    if (!occupied.empty() && emptied.size() >= 1) {
        if (occupied.size() == 1) {
            return MoveCandidate{ emptied.front(), occupied.front() };
        }
    }

    return std::nullopt;
}

} // namespace ChessClock


