#pragma once

#include "core.hpp"
#include "bitboard.hpp"
#include <random>


namespace anka {
    class RNG {
    public:
        using result_type = u64;
        RNG(u64 seed) : m_state{ seed } {}
        RNG() : m_state{ static_cast<u64>(rd()) } {}

        force_inline double rand()
        {
            // source: https://prng.di.unimi.it/
            return (next() >> 11) * 0x1.0p-53;
        }

        force_inline double rand(double min, double max)
        {
            std::uniform_real_distribution<double> distribution(min, max);
            return distribution(*this);
        }
       
        force_inline u64 rand64() { return next(); }
        force_inline u64 rand64_sparse() { return (next() & next() & next()); }
        force_inline u64 rand64(u64 min, u64 max)
        {
            if (min >= max)
                return max;
            u64 mask = UINT64_MAX;
            u64 range = max - min;

            mask >>= (bitboard::BitScanReverse(range));

            u64 value = 0;
            do {
                value = rand64() & mask;
            } while (value > range);

            return min + value;
        }
        force_inline u32 rand32() { return static_cast<u32>(next() >> 32); }

        force_inline int randint() { return static_cast<int>(next() >> 32); }
        force_inline int randint(int min, int max) 
        {
            std::uniform_int_distribution<int> distribution(min, max);
            return distribution(*this);
        }

        // C++ UniformRandomBitGenerator interface methods
        force_inline constexpr static result_type min() { return C64(0); }
        force_inline constexpr static result_type max() { return UINT64_MAX; }
        force_inline result_type operator()() { return rand64(); }
    private:

        u64 m_state;
        std::random_device rd;

        // SPLITMIX64
        force_inline u64 next() {
            u64 z = (m_state += 0x9e3779b97f4a7c15);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
            z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
            return z ^ (z >> 31);
        }
    };
}
