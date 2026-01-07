#include "../include/hasher.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <cstring>

void Hasher::ducos1_hash(const std::string& input, uint8_t output[20]) {
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), 
         input.length(), output);
}

bool Hasher::ducos1_compare(const uint8_t hash1[20], const uint8_t hash2[20]) {
    return memcmp(hash1, hash2, 20) == 0;
}

std::string Hasher::bytes_to_hex(const uint8_t* bytes, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

void Hasher::hex_to_bytes(const std::string& hex, uint8_t* bytes) {
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        bytes[i/2] = static_cast<uint8_t>(strtol(byte_str.c_str(), nullptr, 16));
    }
}