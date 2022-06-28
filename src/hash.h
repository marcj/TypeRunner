#pragma once

#include <string>
#include <cstdint>
#include <stddef.h>

namespace ts::hash {

    //https://github.com/ekpyron/xxhashct
    struct xxh64 {
        static constexpr uint64_t hash(const char *p, uint64_t len, uint64_t seed) {
            return finalize((len >= 32 ? h32bytes(p, len, seed) : seed + PRIME5) + len, p + (len & ~0x1F), len & 0x1F);
        }
    private:
        static constexpr uint64_t PRIME1 = 11400714785074694791ULL;
        static constexpr uint64_t PRIME2 = 14029467366897019727ULL;
        static constexpr uint64_t PRIME3 = 1609587929392839161ULL;
        static constexpr uint64_t PRIME4 = 9650029242287828579ULL;
        static constexpr uint64_t PRIME5 = 2870177450012600261ULL;

        static constexpr uint64_t rotl(uint64_t x, int r) {
            return ((x << r) | (x >> (64 - r)));
        }
        static constexpr uint64_t mix1(const uint64_t h, const uint64_t prime, int rshift) {
            return (h ^ (h >> rshift)) * prime;
        }
        static constexpr uint64_t mix2(const uint64_t p, const uint64_t v = 0) {
            return rotl(v + p * PRIME2, 31) * PRIME1;
        }
        static constexpr uint64_t mix3(const uint64_t h, const uint64_t v) {
            return (h ^ mix2(v)) * PRIME1 + PRIME4;
        }
#ifdef XXH64_BIG_ENDIAN
        static constexpr uint32_t endian32 (const char *v) {
        return uint32_t(uint8_t(v[3]))|(uint32_t(uint8_t(v[2]))<<8)
               |(uint32_t(uint8_t(v[1]))<<16)|(uint32_t(uint8_t(v[0]))<<24);
    }

    static constexpr uint64_t endian64 (const char *v)
    {
        return uint64_t(uint8_t(v[7]))|(uint64_t(uint8_t(v[6]))<<8)
               |(uint64_t(uint8_t(v[5]))<<16)|(uint64_t(uint8_t(v[4]))<<24)
               |(uint64_t(uint8_t(v[3]))<<32)|(uint64_t(uint8_t(v[2]))<<40)
               |(uint64_t(uint8_t(v[1]))<<48)|(uint64_t(uint8_t(v[0]))<<56);
    }
#else
        static constexpr uint32_t endian32(const char *v) {
            return uint32_t(uint8_t(v[0])) | (uint32_t(uint8_t(v[1])) << 8)
                   | (uint32_t(uint8_t(v[2])) << 16) | (uint32_t(uint8_t(v[3])) << 24);
        }
        static constexpr uint64_t endian64(const char *v) {
            return uint64_t(uint8_t(v[0])) | (uint64_t(uint8_t(v[1])) << 8)
                   | (uint64_t(uint8_t(v[2])) << 16) | (uint64_t(uint8_t(v[3])) << 24)
                   | (uint64_t(uint8_t(v[4])) << 32) | (uint64_t(uint8_t(v[5])) << 40)
                   | (uint64_t(uint8_t(v[6])) << 48) | (uint64_t(uint8_t(v[7])) << 56);
        }
#endif
        static constexpr uint64_t fetch64(const char *p, const uint64_t v = 0) {
            return mix2(endian64(p), v);
        }
        static constexpr uint64_t fetch32(const char *p) {
            return uint64_t(endian32(p)) * PRIME1;
        }
        static constexpr uint64_t fetch8(const char *p) {
            return uint8_t(*p) * PRIME5;
        }
        static constexpr uint64_t finalize(const uint64_t h, const char *p, uint64_t len) {
            return (len >= 8) ? (finalize(rotl(h ^ fetch64(p), 27) * PRIME1 + PRIME4, p + 8, len - 8)) :
                   ((len >= 4) ? (finalize(rotl(h ^ fetch32(p), 23) * PRIME2 + PRIME3, p + 4, len - 4)) :
                    ((len > 0) ? (finalize(rotl(h ^ fetch8(p), 11) * PRIME1, p + 1, len - 1)) :
                     (mix1(mix1(mix1(h, PRIME2, 33), PRIME3, 29), 1, 32))));
        }
        static constexpr uint64_t h32bytes(const char *p, uint64_t len, const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) {
            return (len >= 32) ? h32bytes(p + 32, len - 32, fetch64(p, v1), fetch64(p + 8, v2), fetch64(p + 16, v3), fetch64(p + 24, v4)) :
                   mix3(mix3(mix3(mix3(rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18), v1), v2), v3), v4);
        }
        static constexpr uint64_t h32bytes(const char *p, uint64_t len, const uint64_t seed) {
            return h32bytes(p, len, seed + PRIME1 + PRIME2, seed + PRIME2, seed, seed - PRIME1);
        }
    };

    inline consteval size_t constLength(const char *str) {
        return (*str == 0) ? 0 : constLength(str + 1) + 1;
    }

    inline size_t length(const char *str) {
        return (*str == 0) ? 0 : length(str + 1) + 1;
    }

    inline consteval uint64_t const_hash(const char *input) {
        return xxh64::hash(input, constLength(input), 0);
    }

    inline consteval uint64_t const_hash(const std::string &input) {
        return const_hash(input.c_str());
    }

    inline uint64_t runtime_hash(const std::string &input) {
        return xxh64::hash(input.c_str(), input.size(), 0);
    }

    inline uint64_t runtime_hash(const std::string_view &input) {
        return xxh64::hash(input.begin(), input.size(), 0);
    }

    inline uint64_t runtime_hash(const char * input) {
        return xxh64::hash(input, length(input), 0);
    }

    inline consteval uint64_t operator ""_hash(const char *s, size_t size) {
        return xxh64::hash(s, size, 0);
    }
};