#include "streamx/utils/config.h"
#include "streamx/utils/logger.h"
#include <fstream>

namespace streamx {

Config::Config() {
}

Config& Config::Instance() {
    static Config instance;
    return instance;
}

bool Config::LoadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) {
        STREAMX_WARN("Config file not found: " + filename);
        return false;
    }

    try {
        file >> data_;
        config_file_ = filename;
        STREAMX_INFO("Config loaded from: " + filename);
        return true;
    } catch (const std::exception& e) {
        STREAMX_ERROR("Failed to parse config file: " + std::string(e.what()));
        return false;
    }
}

bool Config::SaveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(config_mutex_);

    std::ofstream file(filename);
    if (!file.is_open()) {
        STREAMX_ERROR("Failed to open config file for writing: " + filename);
        return false;
    }

    try {
        file << data_.dump(4) << std::endl;
        STREAMX_INFO("Config saved to: " + filename);
        return true;
    } catch (const std::exception& e) {
        STREAMX_ERROR("Failed to save config: " + std::string(e.what()));
        return false;
    }
}

}  // namespace streamx
