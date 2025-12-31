#pragma once

#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>

namespace ChessClock {

enum class LogLevel { INFO, WARN, ERR, DEBUG };

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Main logging function
    void log(LogLevel level, const std::string& tag, const std::string& message);

    // Convenience macros or static wrappers
    static void info(const std::string& tag, const std::string& msg) { 
        getInstance().log(LogLevel::INFO, tag, msg); 
    }

private:
    Logger();
    ~Logger();
    void processQueue();

    std::ofstream m_file;
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_logThread;
    std::atomic<bool> m_running;
};

} // namespace ChessClock