#ifndef HASHER_H
#define HASHER_H

#include <string>
#include <cstdint>
#include <cstring>

// x86 SIMD
#if defined(USE_AVX512) || defined(USE_AVX2)
#include <immintrin.h>
#endif

// ARM NEON
#if defined(USE_ARM_NEON)
#include <arm_neon.h>
#endif

class Hasher {
public:
    static void ducos1_hash(const std::string& input, uint8_t output[20]);
    static bool ducos1_compare(const uint8_t hash1[20], const uint8_t hash2[20]);
    static std::string bytes_to_hex(const uint8_t* bytes, size_t len);
    static void hex_to_bytes(const std::string& hex, uint8_t* bytes);
    
#if defined(USE_AVX2)
    static bool ducos1_compare_avx2(const uint8_t hash1[20], const uint8_t hash2[20]);
#endif

#if defined(USE_AVX512)
    static bool ducos1_compare_avx512(const uint8_t hash1[20], const uint8_t hash2[20]);
#endif

#if defined(USE_ARM_NEON)
    static bool ducos1_compare_neon(const uint8_t hash1[20], const uint8_t hash2[20]);
#endif
};

#endif