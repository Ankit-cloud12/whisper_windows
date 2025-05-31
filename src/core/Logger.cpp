/*
 * Logger.cpp
 *
 * Implementation of the Logger class.
 */

#include "Logger.h"
#include <iostream>
#include <filesystem>
#include <ctime>
#include <condition_variable>

#ifdef _WIN32
#include <windows.h>
#endif

namespace WhisperApp {

namespace fs = std::filesystem;

/**
 * @brief Private implementation class
 */
class Logger::Impl {
public:
    LoggerConfig config;
    LogMetrics metrics;
    
    // File logging
    std::ofstream logFile;
    std::string currentLogPath;
    size_t currentFileSize = 0;
    
    // Async logging
    std::queue<LogEntry> logQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::thread workerThread;
    std::atomic<bool> shouldStop{false};
    
    // Synchronization
    std::mutex consoleMutex;
    std::mutex fileMutex;
    
    Impl() {}
    
    ~Impl() {
        stop();
    }
    
    void start() {
        if (config.asyncLogging) {
            workerThread = std::thread(&Impl::processLogs, this);
        }
        
        if (config.enableFile) {
            ensureLogDirectory();
            rotateLogFile();
        }
    }
    
    void stop() {
        shouldStop = true;
        queueCondition.notify_all();
        
        if (workerThread.joinable()) {
            workerThread.join();
        }
        
        // Process any remaining logs
        while (!logQueue.empty()) {
            processLogEntry(logQueue.front());
            logQueue.pop();
        }
        
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void processLogs() {
        while (!shouldStop) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] { 
                return !logQueue.empty() || shouldStop; 
            });
            
            while (!logQueue.empty()) {
                LogEntry entry = logQueue.front();
                logQueue.pop();
                lock.unlock();
                
                processLogEntry(entry);
                
                lock.lock();
            }
        }
    }
    
    void processLogEntry(const LogEntry& entry) {
        // Console output
        if (config.enableConsole && entry.level >= config.consoleLevel) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            writeToConsole(entry);
        }
        
        // File output
        if (config.enableFile && entry.level >= config.fileLevel) {
            std::lock_guard<std::mutex> lock(fileMutex);
            writeToFile(entry);
        }
        
        metrics.totalLogs++;
    }
    
    void writeToConsole(const LogEntry& entry) {
        std::ostream& out = (entry.level >= LogLevel::ERROR) ? std::cerr : std::cout;
        
        // Set console color based on log level
        setConsoleColor(entry.level);
        
        out << formatLogEntry(entry) << std::endl;
        
        // Reset console color
        resetConsoleColor();
    }
    
    void writeToFile(const LogEntry& entry) {
        if (!logFile.is_open()) {
            return;
        }
        
        std::string formatted = formatLogEntry(entry);
        logFile << formatted << std::endl;
        logFile.flush();
        
        currentFileSize += formatted.length() + 1;
        
        // Check if rotation is needed
        if (currentFileSize >= config.maxFileSize) {
            rotateLogFile();
        }
    }
    
    std::string formatLogEntry(const LogEntry& entry) {
        std::ostringstream oss;
        
        // Timestamp
        if (config.includeTimestamp) {
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                entry.timestamp.time_since_epoch()) % 1000;
            
            oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            oss << "." << std::setfill('0') << std::setw(3) << ms.count() << " ";
        }
        
        // Log level
        oss << "[" << std::setw(5) << Logger::levelToString(entry.level) << "] ";
        
        // Thread ID
        if (config.includeThreadId) {
            oss << "[" << entry.threadId << "] ";
        }
        
        // Module
        if (config.includeModule && !entry.module.empty()) {
            oss << "[" << entry.module << "] ";
        }
        
        // Message
        oss << entry.message;
        
        return oss.str();
    }
    
    void ensureLogDirectory() {
        if (!fs::exists(config.logDirectory)) {
            fs::create_directories(config.logDirectory);
        }
    }
    
    void rotateLogFile() {
        if (logFile.is_open()) {
            logFile.close();
        }
        
        // Generate new log filename
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << config.logDirectory << "/" << config.logFilePrefix << "_";
        oss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        oss << ".log";
        
        currentLogPath = oss.str();
        logFile.open(currentLogPath, std::ios::out | std::ios::app);
        currentFileSize = 0;
        
        metrics.filesRotated++;
        
        // Clean up old log files
        cleanupOldLogs();
    }
    
    void cleanupOldLogs() {
        std::vector<fs::path> logFiles;
        
        for (const auto& entry : fs::directory_iterator(config.logDirectory)) {
            if (entry.is_regular_file() && 
                entry.path().filename().string().find(config.logFilePrefix) == 0) {
                logFiles.push_back(entry.path());
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(logFiles.begin(), logFiles.end(), 
            [](const fs::path& a, const fs::path& b) {
                return fs::last_write_time(a) > fs::last_write_time(b);
            });
        
        // Remove old files
        if (logFiles.size() > config.maxFiles) {
            for (size_t i = config.maxFiles; i < logFiles.size(); ++i) {
                fs::remove(logFiles[i]);
            }
        }
    }
    
    void setConsoleColor(LogLevel level) {
#ifdef _WIN32
        // Windows console color codes
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        switch (level) {
            case LogLevel::DEBUG:
                SetConsoleTextAttribute(hConsole, 8);  // Gray
                break;
            case LogLevel::INFO:
                SetConsoleTextAttribute(hConsole, 7);  // White
                break;
            case LogLevel::WARN:
                SetConsoleTextAttribute(hConsole, 14); // Yellow
                break;
            case LogLevel::ERROR:
                SetConsoleTextAttribute(hConsole, 12); // Red
                break;
            case LogLevel::FATAL:
                SetConsoleTextAttribute(hConsole, 79); // White on Red
                break;
            default:
                break;
        }
#else
        // ANSI color codes for Unix-like systems
        switch (level) {
            case LogLevel::DEBUG:
                std::cout << "\033[90m";  // Gray
                break;
            case LogLevel::INFO:
                std::cout << "\033[0m";   // Default
                break;
            case LogLevel::WARN:
                std::cout << "\033[33m";  // Yellow
                break;
            case LogLevel::ERROR:
                std::cout << "\033[31m";  // Red
                break;
            case LogLevel::FATAL:
                std::cout << "\033[41m";  // Red background
                break;
            default:
                break;
        }
#endif
    }
    
    void resetConsoleColor() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 7); // Default white
#else
        std::cout << "\033[0m"; // Reset
#endif
    }
};

// Logger implementation

Logger::Logger() : pImpl(std::make_unique<Impl>()) {}

Logger::~Logger() = default;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const LoggerConfig& config) {
    pImpl->config = config;
    pImpl->start();
}

void Logger::shutdown() {
    pImpl->stop();
}

void Logger::log(LogLevel level, const std::string& module, const std::string& message) {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.module = module;
    entry.message = message;
    entry.threadId = std::this_thread::get_id();
    
    if (pImpl->config.asyncLogging) {
        std::lock_guard<std::mutex> lock(pImpl->queueMutex);
        
        // Drop logs if queue is too large
        if (pImpl->logQueue.size() > 10000) {
            pImpl->metrics.droppedLogs++;
            return;
        }
        
        pImpl->logQueue.push(entry);
        pImpl->queueCondition.notify_one();
    } else {
        pImpl->processLogEntry(entry);
    }
}

void Logger::setConsoleLevel(LogLevel level) {
    pImpl->config.consoleLevel = level;
}

void Logger::setFileLevel(LogLevel level) {
    pImpl->config.fileLevel = level;
}

void Logger::setConsoleEnabled(bool enabled) {
    pImpl->config.enableConsole = enabled;
}

void Logger::setFileEnabled(bool enabled) {
    pImpl->config.enableFile = enabled;
}

void Logger::flush() {
    if (pImpl->config.asyncLogging) {
        // Wait for queue to empty
        std::unique_lock<std::mutex> lock(pImpl->queueMutex);
        pImpl->queueCondition.wait(lock, [this] { 
            return pImpl->logQueue.empty(); 
        });
    }
    
    if (pImpl->logFile.is_open()) {
        pImpl->logFile.flush();
    }
}

LogMetrics Logger::getMetrics() const {
    LogMetrics metrics;
    metrics.totalLogs.store(pImpl->metrics.totalLogs.load());
    metrics.droppedLogs.store(pImpl->metrics.droppedLogs.load());
    metrics.filesRotated.store(pImpl->metrics.filesRotated.load());
    metrics.startTime = pImpl->metrics.startTime;
    return metrics;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const std::string& level) {
    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO")  return LogLevel::INFO;
    if (level == "WARN")  return LogLevel::WARN;
    if (level == "ERROR") return LogLevel::ERROR;
    if (level == "FATAL") return LogLevel::FATAL;
    return LogLevel::INFO;
}

// LogTimer implementation

LogTimer::LogTimer(const std::string& module, const std::string& operation)
    : module_(module), operation_(operation), start_(std::chrono::steady_clock::now()) {
    Logger::getInstance().debug(module_, "Starting: " + operation_);
}

LogTimer::~LogTimer() {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
    
    std::ostringstream oss;
    oss << "Completed: " << operation_ << " (took " << duration.count() << " ms)";
    Logger::getInstance().debug(module_, oss.str());
}

} // namespace WhisperApp