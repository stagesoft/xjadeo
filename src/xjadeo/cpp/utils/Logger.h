#ifndef XJADEO_LOGGER_H
#define XJADEO_LOGGER_H

#include <string>
#include <iostream>
#include <sstream>

namespace xjadeo {

/**
 * Logger - Simple logging utility for C++ code
 * 
 * Provides consistent logging interface that can integrate with
 * existing C code's logging (want_quiet, want_verbose, want_debug).
 */
class Logger {
public:
    enum Level {
        ERROR = 0,
        WARNING = 1,
        INFO = 2,
        DEBUG = 3,
        VERBOSE = 4
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(Level level) { level_ = level; }
    Level getLevel() const { return level_; }

    void setQuiet(bool quiet) { quiet_ = quiet; }
    bool isQuiet() const { return quiet_; }

    // Logging methods
    void error(const std::string& message) {
        if (!quiet_ && level_ >= ERROR) {
            std::cerr << "[ERROR] " << message << std::endl;
        }
    }

    void warning(const std::string& message) {
        if (!quiet_ && level_ >= WARNING) {
            std::cerr << "[WARNING] " << message << std::endl;
        }
    }

    void info(const std::string& message) {
        if (!quiet_ && level_ >= INFO) {
            std::cout << "[INFO] " << message << std::endl;
        }
    }

    void debug(const std::string& message) {
        if (!quiet_ && level_ >= DEBUG) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }

    void verbose(const std::string& message) {
        if (!quiet_ && level_ >= VERBOSE) {
            std::cout << "[VERBOSE] " << message << std::endl;
        }
    }

    // Convenience macros (can be used like: LOG_INFO << "message")
    class LogStream {
    public:
        LogStream(Logger& logger, Level level, bool shouldLog)
            : logger_(logger), level_(level), shouldLog_(shouldLog) {}
        
        // Move constructor (needed because ostringstream is not copyable)
        LogStream(LogStream&& other) noexcept
            : logger_(other.logger_), level_(other.level_), shouldLog_(other.shouldLog_),
              stream_(std::move(other.stream_)) {}
        
        // Delete copy constructor
        LogStream(const LogStream&) = delete;
        LogStream& operator=(const LogStream&) = delete;
        
        ~LogStream() {
            if (shouldLog_) {
                std::string msg = stream_.str();
                switch (level_) {
                    case ERROR: logger_.error(msg); break;
                    case WARNING: logger_.warning(msg); break;
                    case INFO: logger_.info(msg); break;
                    case DEBUG: logger_.debug(msg); break;
                    case VERBOSE: logger_.verbose(msg); break;
                }
            }
        }

        template<typename T>
        LogStream& operator<<(const T& value) {
            if (shouldLog_) {
                stream_ << value;
            }
            return *this;
        }

    private:
        Logger& logger_;
        Level level_;
        bool shouldLog_;
        std::ostringstream stream_;
    };

    LogStream error() { return LogStream(*this, ERROR, !quiet_ && level_ >= ERROR); }
    LogStream warning() { return LogStream(*this, WARNING, !quiet_ && level_ >= WARNING); }
    LogStream info() { return LogStream(*this, INFO, !quiet_ && level_ >= INFO); }
    LogStream debug() { return LogStream(*this, DEBUG, !quiet_ && level_ >= DEBUG); }
    LogStream verbose() { return LogStream(*this, VERBOSE, !quiet_ && level_ >= VERBOSE); }

private:
    Logger() : level_(INFO), quiet_(false) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Level level_;
    bool quiet_;
};

// Convenience macros
#define LOG_ERROR   xjadeo::Logger::getInstance().error()
#define LOG_WARNING xjadeo::Logger::getInstance().warning()
#define LOG_INFO    xjadeo::Logger::getInstance().info()
#define LOG_DEBUG   xjadeo::Logger::getInstance().debug()
#define LOG_VERBOSE xjadeo::Logger::getInstance().verbose()

} // namespace xjadeo

#endif // XJADEO_LOGGER_H

