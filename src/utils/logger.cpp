#include "streamx/utils/logger.h"
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace streamx {

struct Logger::Impl {
    std::ofstream log_file;
};

Logger::Logger() 
    : current_level_(LogLevel::Info), impl_(std::make_unique<Impl>()) {
}

Logger::~Logger() {
    Close();
}

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::string& log_file, LogLevel level) {
    current_level_ = level;
    log_file_ = log_file;

    if (!log_file_.empty()) {
        impl_->log_file.open(log_file_, std::ios::app);
        if (impl_->log_file.is_open()) {
            use_file_ = true;
        }
    }

    is_initialized_ = true;
    
    // Log after initialization to avoid deadlock
    Info("=== StreamX Logger Initialized ===");
}

void Logger::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    current_level_ = level;
}

void Logger::Close() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (impl_->log_file.is_open()) {
        Info("=== StreamX Logger Closed ===");
        impl_->log_file.close();
    }
}

static std::string LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

static std::string GetTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < current_level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex_);

    std::string log_message = "[" + GetTimestamp() + "] [" + LevelToString(level) + "] " + message;

    // Console output
    std::cout << log_message << std::endl;

    // File output
    if (use_file_ && impl_->log_file.is_open()) {
        impl_->log_file << log_message << std::endl;
        impl_->log_file.flush();
    }
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::Debug, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::Info, message);
}

void Logger::Warning(const std::string& message) {
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::Error, message);
}

void Logger::Critical(const std::string& message) {
    Log(LogLevel::Critical, message);
}

void Logger::Debugf(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Debug(std::string(buffer));
}

void Logger::Infof(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Info(std::string(buffer));
}

void Logger::Warningf(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Warning(std::string(buffer));
}

void Logger::Errorf(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Error(std::string(buffer));
}

void Logger::Criticalf(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Critical(std::string(buffer));
}

}  // namespace streamx
