#include "logger/Logger.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>

namespace ChessClock {

Logger::Logger() : m_running(true) {
    m_file.open("chess_clock.log", std::ios::out | std::ios::app);
    m_logThread = std::thread(&Logger::processQueue, this);
}

Logger::~Logger() {
    m_running = false;
    m_cv.notify_all();
    if (m_logThread.joinable()) m_logThread.join();
    if (m_file.is_open()) m_file.close();
}

void Logger::log(LogLevel level, const std::string& tag, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // Format the prefix: [TIME][LEVEL][TAG]
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time_t_now), "%H:%M:%S") << "]";
    
    switch(level) {
        case LogLevel::INFO:  ss << "[INFO]"; break;
        case LogLevel::DEBUG: ss << "[DEBUG]"; break;
        case LogLevel::ERR:   ss << "[ERROR]"; break;
        default: break;
    }
    
    ss << "[" << tag << "] " << message << std::endl;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(ss.str());
    }
    m_cv.notify_one();
}

void Logger::processQueue() {
    while (m_running || !m_queue.empty()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });

        while (!m_queue.empty()) {
            std::string msg = m_queue.front();
            m_queue.pop();
            
            // Write to file
            if (m_file.is_open()) m_file << msg;
            
            // Also write to console for GDB visibility
            std::cout << msg << std::flush;
        }
    }
}

} // namespace ChessClock