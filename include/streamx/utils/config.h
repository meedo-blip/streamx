#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace streamx {

using json = nlohmann::json;

// Configuration manager for loading/saving settings with encryption
class Config {
public:
    static Config& Instance();

    // Load configuration from file (auto-decrypts if encrypted)
    bool LoadFromFile(const std::string& filename);
    
    // Save configuration to file (auto-encrypts)
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

    bool Contains(const std::string& key) const {
        return data_.contains(key);
    }

    json& operator[](const std::string& key) {
        return data_[key];
    }

    const json& operator[](const std::string& key) const {
        return data_[key];
    }

    const json& GetRaw() const { return data_; }
    void SetRaw(const json& data) { data_ = data; }

private:
    Config();

    // Encryption helpers using OpenSSL AES-256-CBC
    std::string Encrypt(const std::string& plaintext) const;
    std::string Decrypt(const std::string& ciphertext) const;
    std::string GetEncryptionKey() const;

    json data_;
    std::string config_file_;
    mutable std::mutex config_mutex_;
};

}  // namespace streamx