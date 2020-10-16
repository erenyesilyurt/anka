#pragma once

#include "core.hpp"
#include "bitboard.hpp"
#include <random>


// An implementation of Xoroshiro128++ described by David Blackman and Sebastiano Vigna (http://prng.di.unimi.it/)
namespace anka {
    class RNG {
    public:
        using result_type = u64;
        RNG(u64 seed) {
            seed = SplitMix64(seed);
            s[0] = seed;
            seed = SplitMix64(seed);
            s[1] = seed;
        }
        RNG() {
            u64 x = static_cast<u64>(rd());
            x = SplitMix64(x);
            s[0] = x;
            x = SplitMix64(x);
            s[1] = x;
        }


        force_inline u64 rand64() { return next(); }
        force_inline u64 rand64_sparse() { return (next() & next() & next()); }
        force_inline u64 rand64(u64 min, u64 max)
        {
            u64 mask = UINT64_MAX;
            u64 range = max - min;

            mask >>= (bitboard::BitScanReverse(range));

            u64 value = 0;
            do {
                value = rand64() & mask;
            } while (value > range);

            return min + value;
        }
        force_inline int32_t rand32() { return static_cast<int32_t>(next() >> 32); }

        // C++ UniformRandomBitGenerator interface methods
        force_inline static result_type min() { return C64(0); }
        force_inline static result_type max() { return UINT64_MAX; }
        force_inline result_type operator()() { return rand64(); }
    private:
        u64 s[2];
        force_inline u64 rotl(const u64 x, int k) {
            return (x << k) | (x >> (64 - k));
        }
        std::random_device rd;

        // SPLITMIX64, used to initialize state s[]
        force_inline u64 SplitMix64(u64 x) {
            u64 z = (x += 0x9e3779b97f4a7c15);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
            z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
            return z ^ (z >> 31);
        }

        force_inline u64 next() {
            const u64 s0 = s[0];
            u64 s1 = s[1];
            const u64 result = s0 + s1;

            s1 ^= s0;
            s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16);  // a, b
            s[1] = rotl(s1, 37);                    // c

            return result;
        }
    };
}
