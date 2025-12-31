#pragma once

#include <memory>

namespace ChessClock {

/**
 * @brief IMoveProcessor defines the contract for an engine that consumes
 * perception data and translates it into game logic updates.
 */
class IMoveProcessor {
public:
    // Virtual destructor is mandatory for interfaces to ensure 
    // derived classes are cleaned up properly when deleted via this pointer.
    virtual ~IMoveProcessor() = default;

    /**
     * @brief Setup internal resources. 
     * @return true if dependencies (Perception/FSM) are valid.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Spawn the background processing thread.
     */
    virtual void start() = 0;

    /**
     * @brief Signal the thread to stop and join it.
     */
    virtual void stop() = 0;

    /**
     * @brief Check if the processing loop is currently active.
     */
    virtual bool isRunning() const = 0;


    IMoveProcessor(const IMoveProcessor&) = delete;
    IMoveProcessor& operator=(const IMoveProcessor&) = delete;

protected:
    IMoveProcessor() = default;
};

} // namespace ChessClock