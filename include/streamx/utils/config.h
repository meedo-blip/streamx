#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace streamx {

using json = nlohmann::json;

// Configuration manager for loading/saving settings
class Config {
public:
    static Config& Instance();

    // Load configuration from file
    bool LoadFromFile(const std::string& filename);
    
    // Save configuration to file
    bool SaveToFile(const std::string& filename) const;

    // Get/Set values
    template<typename T>
    T Get(const std::string& key, const T& default_value = T()) const {
        try {
            if (data_.contains(key)) {
                return data_[key].get<T>();
            }
        } catch (...) {
        }
        return default_value;
    }

    template<typename T>
    void Set(const std::string& key, const T& value) {
        data_[key] = value;
    }

    bool Has(const std::string& key) const {
        return data_.contains(key);
    }

    void Remove(const std::string& key) {
        data_.erase(key);
    }

    void Clear() {
        data_.clear();
    }

    const json& GetRaw() const { return data_; }
    void SetRaw(const json& data) { data_ = data; }

private:
    Config();
    json data_;
    std::string config_file_;
    mutable std::mutex config_mutex_;
};

}  // namespace streamx
