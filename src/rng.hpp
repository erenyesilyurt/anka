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
            std::uniform_real_distribution<double> distribution(0.0, 1.0);
            return distribution(*this);
        }

        force_inline double rand(double min, double max)
        {
            std::uniform_real_distribution<double> distribution(min, max);
            return distribution(*this);
        }

        force_inline int32_t rand32() { return static_cast<int32_t>(next() >> 32); }
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


        // C++ UniformRandomBitGenerator interface methods
        force_inline static result_type min() { return C64(0); }
        force_inline static result_type max() { return UINT64_MAX; }
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
