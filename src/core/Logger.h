/*
 * Logger.h
 * 
 * Multi-level logging system for WhisperApp.
 * Supports file and console output with rotating log files.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace WhisperApp {

/**
 * @brief Log levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4,
    NONE = 5
};

/**
 * @brief Log entry structure
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string module;
    std::string message;
    std::thread::id threadId;
};

/**
 * @brief Logger configuration
 */
struct LoggerConfig {
    LogLevel consoleLevel = LogLevel::INFO;
    LogLevel fileLevel = LogLevel::DEBUG;
    std::string logDirectory = "logs";
    std::string logFilePrefix = "whisperapp";
    size_t maxFileSize = 10 * 1024 * 1024;  // 10MB
    size_t maxFiles = 5;
    bool enableConsole = true;
    bool enableFile = true;
    bool asyncLogging = true;
    bool includeTimestamp = true;
    bool includeThreadId = true;
    bool includeModule = true;
};

/**
 * @brief Performance metrics for logging
 */
struct LogMetrics {
    std::atomic<uint64_t> totalLogs{0};
    std::atomic<uint64_t> droppedLogs{0};
    std::atomic<uint64_t> filesRotated{0};
    std::chrono::steady_clock::time_point startTime;
    
    LogMetrics() : startTime(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Singleton logger class
 */
class Logger {
public:
    /**
     * @brief Get logger instance
     */
    static Logger& getInstance();
    
    /**
     * @brief Initialize logger with configuration
     */
    void initialize(const LoggerConfig& config);
    
    /**
     * @brief Shutdown logger
     */
    void shutdown();
    
    /**
     * @brief Log a message
     */
    void log(LogLevel level, const std::string& module, const std::string& message);
    
    /**
     * @brief Log with formatting (printf-style)
     */
    template<typename... Args>
    void logf(LogLevel level, const std::string& module, const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        log(level, module, std::string(buffer));
    }
    
    /**
     * @brief Convenience methods for different log levels
     */
    void debug(const std::string& module, const std::string& message) {
        log(LogLevel::DEBUG, module, message);
    }
    
    void info(const std::string& module, const std::string& message) {
        log(LogLevel::INFO, module, message);
    }
    
    void warn(const std::string& module, const std::string& message) {
        log(LogLevel::WARN, module, message);
    }
    
    void error(const std::string& module, const std::string& message) {
        log(LogLevel::ERROR, module, message);
    }
    
    void fatal(const std::string& module, const std::string& message) {
        log(LogLevel::FATAL, module, message);
    }
    
    /**
     * @brief Set log level for console output
     */
    void setConsoleLevel(LogLevel level);
    
    /**
     * @brief Set log level for file output
     */
    void setFileLevel(LogLevel level);
    
    /**
     * @brief Enable/disable console output
     */
    void setConsoleEnabled(bool enabled);
    
    /**
     * @brief Enable/disable file output
     */
    void setFileEnabled(bool enabled);
    
    /**
     * @brief Flush all pending logs
     */
    void flush();
    
    /**
     * @brief Get logging metrics
     */
    LogMetrics getMetrics() const;
    
    /**
     * @brief Convert log level to string
     */
    static std::string levelToString(LogLevel level);
    
    /**
     * @brief Convert string to log level
     */
    static LogLevel stringToLevel(const std::string& level);
    
private:
    Logger();
    ~Logger();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Scoped log timer for performance measurement
 */
class LogTimer {
public:
    LogTimer(const std::string& module, const std::string& operation);
    ~LogTimer();
    
private:
    std::string module_;
    std::string operation_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * @brief Convenience macros for logging
 */
#define LOG_DEBUG(module, message) \
    WhisperApp::Logger::getInstance().debug(module, message)

#define LOG_INFO(module, message) \
    WhisperApp::Logger::getInstance().info(module, message)

#define LOG_WARN(module, message) \
    WhisperApp::Logger::getInstance().warn(module, message)

#define LOG_ERROR(module, message) \
    WhisperApp::Logger::getInstance().error(module, message)

#define LOG_FATAL(module, message) \
    WhisperApp::Logger::getInstance().fatal(module, message)

#define LOG_TIMER(module, operation) \
    WhisperApp::LogTimer _timer(module, operation)

} // namespace WhisperApp

#endif // LOGGER_H