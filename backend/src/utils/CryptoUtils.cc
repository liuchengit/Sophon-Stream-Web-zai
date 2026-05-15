#include "utils/CryptoUtils.h"
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <random>

namespace sophon_stream {
namespace web {
namespace utils {

bool CryptoUtils::sha256(const unsigned char* data, size_t length, unsigned char* output) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        spdlog::error("CryptoUtils: Failed to create EVP_MD_CTX");
        return false;
    }

    bool success = false;
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, data, length) == 1 &&
        EVP_DigestFinal_ex(ctx, output, nullptr) == 1) {
        success = true;
    } else {
        spdlog::error("CryptoUtils: SHA-256 computation failed");
    }

    EVP_MD_CTX_free(ctx);
    return success;
}

bool CryptoUtils::randomBytes(unsigned char* buffer, size_t length) {
    if (RAND_bytes(buffer, static_cast<int>(length)) != 1) {
        spdlog::error("CryptoUtils: RAND_bytes failed");
        return false;
    }
    return true;
}

std::string CryptoUtils::toHex(const unsigned char* data, size_t length) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string CryptoUtils::hashPassword(const std::string& password, const std::string& salt) {
    // Combine salt + password for hashing: salt|password (prevents rainbow table attacks)
    std::string combined = salt + "|" + password;

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        spdlog::error("CryptoUtils: Failed to create EVP_MD_CTX for password hashing");
        return "";
    }

    std::string result;
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, combined.data(), combined.size()) == 1 &&
        EVP_DigestFinal_ex(ctx, hash, &hashLen) == 1) {
        result = toHex(hash, hashLen);
    } else {
        spdlog::error("CryptoUtils: Password hash computation failed");
    }

    EVP_MD_CTX_free(ctx);

    // Additional iterations for key stretching (3 rounds total)
    for (int i = 0; i < 2 && !result.empty(); ++i) {
        std::string input = result + salt;
        unsigned char iterHash[EVP_MAX_MD_SIZE];
        unsigned int iterLen = 0;

        EVP_MD_CTX* iterCtx = EVP_MD_CTX_new();
        if (!iterCtx) {
            spdlog::error("CryptoUtils: Failed to create EVP_MD_CTX for iteration");
            return "";
        }

        if (EVP_DigestInit_ex(iterCtx, EVP_sha256(), nullptr) == 1 &&
            EVP_DigestUpdate(iterCtx, input.data(), input.size()) == 1 &&
            EVP_DigestFinal_ex(iterCtx, iterHash, &iterLen) == 1) {
            result = toHex(iterHash, iterLen);
        } else {
            spdlog::error("CryptoUtils: Password hash iteration failed");
            result = "";
        }

        EVP_MD_CTX_free(iterCtx);
    }

    return result;
}

std::string CryptoUtils::generateSalt(int length) {
    std::vector<unsigned char> buffer(length);
    if (!randomBytes(buffer.data(), length)) {
        // Fallback to a less secure but functional approach
        spdlog::warn("CryptoUtils: Using fallback salt generation");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 0; i < length; ++i) {
            buffer[i] = static_cast<unsigned char>(dis(gen));
        }
    }
    return toHex(buffer.data(), length);
}

std::string CryptoUtils::generateApiKey() {
    // Generate UUID v4
    unsigned char bytes[16];
    if (!randomBytes(bytes, 16)) {
        spdlog::warn("CryptoUtils: Using fallback UUID generation");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 0; i < 16; ++i) {
            bytes[i] = static_cast<unsigned char>(dis(gen));
        }
    }

    // Set version to 4 (random UUID)
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    // Set variant to RFC 4122
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    // Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    oss << std::setw(2) << static_cast<int>(bytes[0])
        << std::setw(2) << static_cast<int>(bytes[1])
        << std::setw(2) << static_cast<int>(bytes[2])
        << std::setw(2) << static_cast<int>(bytes[3])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[4])
        << std::setw(2) << static_cast<int>(bytes[5])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[6])
        << std::setw(2) << static_cast<int>(bytes[7])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[8])
        << std::setw(2) << static_cast<int>(bytes[9])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[10])
        << std::setw(2) << static_cast<int>(bytes[11])
        << std::setw(2) << static_cast<int>(bytes[12])
        << std::setw(2) << static_cast<int>(bytes[13])
        << std::setw(2) << static_cast<int>(bytes[14])
        << std::setw(2) << static_cast<int>(bytes[15]);

    return oss.str();
}

std::string CryptoUtils::generateRandomHex(int byteLength) {
    std::vector<unsigned char> buffer(byteLength);
    if (!randomBytes(buffer.data(), byteLength)) {
        spdlog::warn("CryptoUtils: Using fallback random generation");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 0; i < byteLength; ++i) {
            buffer[i] = static_cast<unsigned char>(dis(gen));
        }
    }
    return toHex(buffer.data(), byteLength);
}

bool CryptoUtils::constantTimeCompare(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }

    volatile unsigned char result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        result |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    }

    return result == 0;
}

}  // namespace utils
}  // namespace web
}  // namespace sophon_stream
