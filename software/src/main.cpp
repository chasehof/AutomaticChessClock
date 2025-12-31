#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "core/ClockEngine.hpp"
#include "perception/OpenCVPerceptionEngine.hpp"
#include "core/MoveProcessor.hpp"
#include "core/MoveFSM.hpp"
#include "logger/Logger.hpp"

using namespace ChessClock;
using namespace std::chrono_literals;

// Global flag for clean shutdown (handles Ctrl+C)
static std::atomic<bool> g_applicationRunning{true};

void signalHandler(int sig) {
    Logger::info("main", "Received signal " + std::to_string(sig) + ", shutting down...");
    g_applicationRunning = false;
}

int main() {
    ChessClock::Logger::info("main", "Starting AutomaticChessClock");
    // 1. Setup Linux Signal Handling
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 2. Initialize Core Engines
    // We set up the "brain" and "body" before we check the hardware
    ClockEngine::Config clockConfig{10min, 0s};
    auto clock = std::make_shared<ClockEngine>(clockConfig);
    auto perception = std::make_shared<OpenCVPerceptionEngine>();

    auto moveCb = [clock](const MoveConfirmedEvent& e) { 
        clock->onMoveConfirmed(e); 
    };

    auto fsm = std::make_unique<MoveFSM>(std::move(moveCb));
    auto processor = std::make_unique<MoveProcessor>(perception, std::move(fsm));

    // Wire up the shutdown callbacks so threads check g_applicationRunning
    auto shutdownCheck = []() { return !g_applicationRunning.load(); };
    perception->setShutdownCallback(shutdownCheck);
    processor->setShutdownCallback(shutdownCheck);

    // ---------------------------------------------------------
    // STATE 1: Hardware Initialization
    // ---------------------------------------------------------
    while (g_applicationRunning && !perception->initialize()) {
        // In a production app, this would update a UI "Hardware Error" screen
        std::this_thread::sleep_for(1s); 
    }

    // ---------------------------------------------------------
    // STATE 2: Iterative Calibration
    // ---------------------------------------------------------
    // We don't proceed until the board is correctly seen by OpenCV
    while (g_applicationRunning && !perception->isCalibrated()) {
        if (!perception->calibrate()) {
            // Calibration might fail because:
            // 1. Hands are in the frame
            // 2. Lighting is too poor
            // 3. Board is not fully visible
            std::this_thread::sleep_for(500ms);
            continue; 
        }
    }

    // ---------------------------------------------------------
    // STATE 3: Wait for User Start (Preview Phase)
    // ---------------------------------------------------------
    // Show a simple preview window where user can verify occupancy detection
    // Press 'q' or Enter in the preview window to start the game
    if (g_applicationRunning) {
        Logger::info("main", "Calibration complete. Running preview...");
        Logger::info("main", "Press 'q' or Enter in preview window to start game");
        
        // This runs in main thread - simple and reliable
        perception->runPreviewLoop();
    }

    // ---------------------------------------------------------
    // STATE 4: Service Startup
    // ---------------------------------------------------------
    if (g_applicationRunning && processor->initialize()) {
        processor->start();
        clock->start(); 
        Logger::info("main", "Game started!");
    }

    // ---------------------------------------------------------
    // STATE 5: The Game Loop
    // ---------------------------------------------------------
    while (g_applicationRunning && processor->isRunning()) {
        // The clock ticks in its own thread, but we check for Game Over here
        auto state = clock->getState();
        
        if (state.isGameOver) {
            // Log game over, maybe trigger a "Game Over" UI state
            break;
        }

        // Check for hardware health: if perception fails mid-game, we could 
        // pause the clock or enter a "Re-calibration" state here.
        if (perception->state() == PerceptionState::ERROR) {
             clock->pause();
             // Logic to handle mid-game camera disconnection
        }

        // Throttle this loop to keep CPU usage low on the Pi
        std::this_thread::sleep_for(33ms); // ~30 FPS management rate
    }

    // ---------------------------------------------------------
    // STATE 6: Graceful Shutdown (RAII)
    // ---------------------------------------------------------
    // When g_applicationRunning becomes false or game finishes
    Logger::info("main", "Stopping all components...");
    perception->stop();
    processor->stop();
    clock->pause();
    Logger::info("main", "Shutdown complete");

    return EXIT_SUCCESS;
}