#ifndef HASHER_H
#define HASHER_H

#include <string>
#include <cstdint>

class Hasher {
public:
    static void ducos1_hash(const std::string& input, uint8_t output[20]);
    static bool ducos1_compare(const uint8_t hash1[20], const uint8_t hash2[20]);
    static std::string bytes_to_hex(const uint8_t* bytes, size_t len);
    static void hex_to_bytes(const std::string& hex, uint8_t* bytes);
};

#endif