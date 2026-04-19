#include "streamx/utils/config.h"
#include "streamx/utils/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace streamx {

namespace {

constexpr int AES_KEY_SIZE = 32;  // 256-bit
constexpr int AES_IV_SIZE = 16;  // 128-bit
constexpr int SALT_SIZE = 16;

}  // namespace

Config::Config() {
}

Config& Config::Instance() {
    static Config instance;
    return instance;
}

std::string Config::GetEncryptionKey() const {
    // Get machine-specific key from hardware info
    std::string key_material;

    // Use CPU info if available
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                key_material += line;
                break;
            }
        }
    }

    // Add machine ID
    std::ifstream machine_id("/etc/machine-id");
    if (machine_id.is_open()) {
        std::string id;
        std::getline(machine_id, id);
        key_material += id;
    }

    // Add username for multi-user safety
    if (struct passwd* pw = getpwuid(getuid())) {
        key_material += pw->pw_name;
    }

    // Fallback to username if nothing else
    if (key_material.empty()) {
        key_material = "streamx_default_key";
    }

    // Derive key using simple hash (not ideal but works without extra deps)
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx, key_material.data(), key_material.size());
        EVP_DigestFinal_ex(ctx, hash, &hash_len);
        EVP_MD_CTX_free(ctx);
    }

    return std::string(reinterpret_cast<char*>(hash), AES_KEY_SIZE);
}

std::string Config::Encrypt(const std::string& plaintext) const {
    if (plaintext.empty()) {
        return "";
    }

    std::string key = GetEncryptionKey();
    
    // Generate random IV
    unsigned char iv[AES_IV_SIZE];
    if (!RAND_bytes(iv, AES_IV_SIZE)) {
        STREAMX_ERROR("Failed to generate IV");
        return "";
    }

    // Encrypt using AES-256-CBC
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        STREAMX_ERROR("Failed to create cipher context");
        return "";
    }

    std::string ciphertext;
    ciphertext.resize(plaintext.size() + AES_IV_SIZE);  // space for IV + padding

    int out_len1 = 0;
    int out_len2 = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, 
                        reinterpret_cast<const unsigned char*>(key.data()), iv);
    EVP_EncryptUpdate(ctx, 
                       reinterpret_cast<unsigned char*>(ciphertext.data()), &out_len1,
                       reinterpret_cast<const unsigned char*>(plaintext.data()), 
                       static_cast<int>(plaintext.size()));
    EVP_EncryptFinal_ex(ctx, 
                         reinterpret_cast<unsigned char*>(ciphertext.data() + out_len1), 
                         &out_len2);

    EVP_CIPHER_CTX_free(ctx);

    // Prepend IV to ciphertext - create new string with IV + encrypted data
    size_t total_encrypted_size = out_len1 + out_len2;
    std::string result;
    result.reserve(AES_IV_SIZE + total_encrypted_size);
    result.append(reinterpret_cast<char*>(iv), AES_IV_SIZE);
    result.append(ciphertext.data(), total_encrypted_size);

    // Encode as hex for safe storage
    std::stringstream ss;
    for (unsigned char c : result) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }

    return ss.str();
}

std::string Config::Decrypt(const std::string& encrypted_hex) const {
    if (encrypted_hex.empty()) {
        return "";
    }

    STREAMX_INFO("Decrypt: hex length = " + std::to_string(encrypted_hex.size()));

    // Decode from hex
    std::string ciphertext;
    ciphertext.reserve(encrypted_hex.size() / 2);
    
    for (size_t i = 0; i + 1 < encrypted_hex.size(); i += 2) {
        unsigned int byte;
        std::stringstream ss;
        ss << std::hex << encrypted_hex.substr(i, 2);
        ss >> byte;
        ciphertext.push_back(static_cast<char>(byte));
    }

    STREAMX_INFO("Decrypt: ciphertext length = " + std::to_string(ciphertext.size()));

    if (ciphertext.size() < AES_IV_SIZE) {
        STREAMX_ERROR("Invalid encrypted data - too short");
        return "";
    }

    std::string key = GetEncryptionKey();

    // Extract IV from start
    unsigned char iv[AES_IV_SIZE];
    std::copy(ciphertext.begin(), ciphertext.begin() + AES_IV_SIZE, iv);
    
    std::string encrypted_data = ciphertext.substr(AES_IV_SIZE);

    // Decrypt
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        STREAMX_ERROR("Failed to create cipher context");
        return "";
    }

    std::string plaintext;
    plaintext.resize(encrypted_data.size() + AES_IV_SIZE);

    int out_len1 = 0;
    int out_len2 = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                        reinterpret_cast<const unsigned char*>(key.data()), iv);
    EVP_DecryptUpdate(ctx,
                       reinterpret_cast<unsigned char*>(plaintext.data()), &out_len1,
                       reinterpret_cast<const unsigned char*>(encrypted_data.data()),
                       static_cast<int>(encrypted_data.size()));
    EVP_DecryptFinal_ex(ctx,
                         reinterpret_cast<unsigned char*>(plaintext.data() + out_len1),
                         &out_len2);

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(out_len1 + out_len2);
    STREAMX_INFO("Decrypt: plaintext length = " + std::to_string(plaintext.size()));
    STREAMX_INFO("Decrypt: plaintext = " + plaintext.substr(0, 100) + "...");
    return plaintext;
}

bool Config::LoadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) {
        STREAMX_WARN("Config file not found: " + filename);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    config_file_ = filename;

    // Try to detect if encrypted (starts with "ENCRYPTED:" marker)
    if (content.find("ENCRYPTED:") == 0) {
        std::string encrypted_data = content.substr(10);
        std::string decrypted = Decrypt(encrypted_data);
        if (decrypted.empty()) {
            STREAMX_ERROR("Failed to decrypt config file");
            return false;
        }
        try {
            data_ = json::parse(decrypted);
            STREAMX_INFO("Encrypted config loaded from: " + filename);
            return true;
        } catch (const std::exception& e) {
            STREAMX_ERROR("Failed to parse decrypted config: " + std::string(e.what()));
            return false;
        }
    }

    // Plain JSON
    try {
        data_ = json::parse(content);
        STREAMX_INFO("Config loaded from: " + filename);
        return true;
    } catch (const std::exception& e) {
        STREAMX_ERROR("Failed to parse config file: " + std::string(e.what()));
        return false;
    }
}

bool Config::SaveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // Create directory if needed
    size_t last_slash = filename.rfind('/');
    if (last_slash != std::string::npos) {
        std::string dir = filename.substr(0, last_slash);
        std::string cmd = "mkdir -p " + dir;
        system(cmd.c_str());
    }

    std::string json_data = data_.dump(4);

    // Encrypt the JSON data
    std::string encrypted = Encrypt(json_data);
    if (encrypted.empty()) {
        STREAMX_ERROR("Failed to encrypt config data");
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        STREAMX_ERROR("Failed to open config file for writing: " + filename);
        return false;
    }

    file << "ENCRYPTED:" << encrypted << std::endl;
    STREAMX_INFO("Config saved to: " + filename);
    return true;
}

}  // namespace streamx