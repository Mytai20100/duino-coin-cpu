#include "../include/hasher.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <cstring>

#if defined(__x86_64__) || defined(_M_X64)
#include <cpuid.h>

static bool check_avx2_support() {
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        return (ebx & (1 << 5)) != 0;
    }
    return false;
}

static bool check_avx512_support() {
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        return (ebx & (1 << 16)) != 0;
    }
    return false;
}

static bool check_neon_support() { return false; }

#elif defined(__aarch64__) || defined(_M_ARM64)

static bool check_neon_support() { return true; }
static bool check_avx2_support() { return false; }
static bool check_avx512_support() { return false; }

#else

static bool check_avx2_support() { return false; }
static bool check_avx512_support() { return false; }
static bool check_neon_support() { return false; }

#endif

void Hasher::ducos1_hash(const std::string& input, uint8_t output[20]) {
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), 
         input.length(), output);
}

#if defined(USE_AVX2)
bool Hasher::ducos1_compare_avx2(const uint8_t hash1[20], const uint8_t hash2[20]) {
    alignas(32) uint8_t a_buf[32] = {0};
    alignas(32) uint8_t b_buf[32] = {0};
    memcpy(a_buf, hash1, 20);
    memcpy(b_buf, hash2, 20);
    
    __m256i a = _mm256_load_si256((__m256i*)a_buf);
    __m256i b = _mm256_load_si256((__m256i*)b_buf);
    __m256i cmp = _mm256_cmpeq_epi8(a, b);
    int mask = _mm256_movemask_epi8(cmp);
    
    return (mask & 0xFFFFF) == 0xFFFFF;
}
#endif

#if defined(USE_AVX512)
bool Hasher::ducos1_compare_avx512(const uint8_t hash1[20], const uint8_t hash2[20]) {
    alignas(64) uint8_t a_buf[64] = {0};
    alignas(64) uint8_t b_buf[64] = {0};
    memcpy(a_buf, hash1, 20);
    memcpy(b_buf, hash2, 20);
    
    __m512i a = _mm512_load_si512((__m512i*)a_buf);
    __m512i b = _mm512_load_si512((__m512i*)b_buf);
    __mmask64 mask = _mm512_cmpeq_epi8_mask(a, b);
    
    return (mask & 0xFFFFF) == 0xFFFFF;
}
#endif

#if defined(USE_ARM_NEON)
bool Hasher::ducos1_compare_neon(const uint8_t hash1[20], const uint8_t hash2[20]) {
    uint8x16_t a = vld1q_u8(hash1);
    uint8x16_t b = vld1q_u8(hash2);
    uint8x16_t cmp = vceqq_u8(a, b);
    
    uint64x2_t cmp64 = vreinterpretq_u64_u8(cmp);
    uint64_t low = vgetq_lane_u64(cmp64, 0);
    uint64_t high = vgetq_lane_u64(cmp64, 1);
    
    if (low != 0xFFFFFFFFFFFFFFFFULL || high != 0xFFFFFFFFFFFFFFFFULL) {
        return false;
    }
    
    return memcmp(hash1 + 16, hash2 + 16, 4) == 0;
}
#endif

bool Hasher::ducos1_compare(const uint8_t hash1[20], const uint8_t hash2[20]) {
    static bool detection_done = false;
    static bool cpu_has_avx512 = false;
    static bool cpu_has_avx2 = false;
    static bool cpu_has_neon = false;
    
    if (!detection_done) {
        cpu_has_avx512 = check_avx512_support();
        cpu_has_avx2 = check_avx2_support();
        cpu_has_neon = check_neon_support();
        detection_done = true;
    }
    
#if defined(USE_AVX512)
    if (cpu_has_avx512) {
        return ducos1_compare_avx512(hash1, hash2);
    }
#endif

#if defined(USE_AVX2)
    if (cpu_has_avx2) {
        return ducos1_compare_avx2(hash1, hash2);
    }
#endif

#if defined(USE_ARM_NEON)
    if (cpu_has_neon) {
        return ducos1_compare_neon(hash1, hash2);
    }
#endif
    
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