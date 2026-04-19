#pragma once

#include <string>
#include <memory>
#include <map>
#include <mutex>

namespace streamx {

// Logging levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

// Logger singleton
class Logger {
public:
    static Logger& Instance();

    void Initialize(const std::string& log_file = "", LogLevel level = LogLevel::Info);
    void SetLevel(LogLevel level);
    void Close();

    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Critical(const std::string& message);

    // Printf-style logging
    void Debugf(const char* fmt, ...);
    void Infof(const char* fmt, ...);
    void Warningf(const char* fmt, ...);
    void Errorf(const char* fmt, ...);
    void Criticalf(const char* fmt, ...);

private:
    Logger();
    ~Logger();

    void Log(LogLevel level, const std::string& message);

    LogLevel current_level_;
    std::string log_file_;
    bool use_file_ = false;
    bool is_initialized_ = false;
    std::mutex log_mutex_;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Convenience macros
#define STREAMX_DEBUG(msg) streamx::Logger::Instance().Debug(msg)
#define STREAMX_INFO(msg) streamx::Logger::Instance().Info(msg)
#define STREAMX_WARN(msg) streamx::Logger::Instance().Warning(msg)
#define STREAMX_ERROR(msg) streamx::Logger::Instance().Error(msg)
#define STREAMX_CRITICAL(msg) streamx::Logger::Instance().Critical(msg)

}  // namespace streamx
