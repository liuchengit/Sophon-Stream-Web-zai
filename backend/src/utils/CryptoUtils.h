#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace sophon_stream {
namespace web {
namespace utils {

/**
 * Cryptographic utility functions for password hashing,
 * salt generation, and API key generation using OpenSSL.
 */
class CryptoUtils {
public:
    /**
     * Hash a password with the given salt using SHA-256.
     * @param password The plaintext password.
     * @param salt The salt string.
     * @return Hex-encoded SHA-256 hash.
     */
    static std::string hashPassword(const std::string& password, const std::string& salt);

    /**
     * Generate a cryptographically secure random salt.
     * @param length Number of random bytes (output will be 2*length hex chars).
     * @return Hex-encoded random salt string.
     */
    static std::string generateSalt(int length = 32);

    /**
     * Generate a random UUID v4 string.
     * @return UUID string in format xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx.
     */
    static std::string generateApiKey();

    /**
     * Generate a random hex string of the specified byte length.
     * @param byteLength Number of random bytes.
     * @return Hex-encoded string (2*byteLength characters).
     */
    static std::string generateRandomHex(int byteLength = 16);

    /**
     * Constant-time string comparison to prevent timing attacks.
     * @param a First string.
     * @param b Second string.
     * @return true if strings are equal.
     */
    static bool constantTimeCompare(const std::string& a, const std::string& b);

    /**
     * Encode binary data to hex string.
     * @param data Binary data.
     * @param length Length of data.
     * @return Hex-encoded string.
     */
    static std::string toHex(const unsigned char* data, size_t length);

private:
    /**
     * Compute SHA-256 hash of input data.
     * @param data Input data.
     * @param length Length of input.
     * @param output Output buffer (must be at least 32 bytes).
     * @return true on success.
     */
    static bool sha256(const unsigned char* data, size_t length, unsigned char* output);

    /**
     * Generate cryptographically secure random bytes.
     * @param buffer Output buffer.
     * @param length Number of bytes to generate.
     * @return true on success.
     */
    static bool randomBytes(unsigned char* buffer, size_t length);
};

}  // namespace utils
}  // namespace web
}  // namespace sophon_stream
