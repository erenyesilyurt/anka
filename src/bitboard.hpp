#pragma once
#include "core.hpp"
#include "boarddefs.hpp"
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>

namespace anka {

	typedef uint64_t Bitboard;

	namespace bitboard {

		force_inline void SetBit(u64& bitboard, int index)
		{
			bitboard |= (C64(1) << index);
		}

		force_inline void ClearBit(u64& bitboard, int index)
		{
			bitboard &= ~(C64(1) << index);
		}

		force_inline void FlipBit(u64& bitboard, int index)
		{
			bitboard ^= (C64(1) << index);
		}

		force_inline bool BitIsSet(const u64 bitboard, int index)
		{
			return bitboard & (C64(1) << index);
		}

		// returns the index of LS1B (0..63) of bitboard
		force_inline int BitScanForward(const u64 bitboard)
		{
			return static_cast<int>(_tzcnt_u64(bitboard));
		}

		// returns the index of MS1B of bitboard
		force_inline int BitScanReverse(const u64 bitboard)
		{
			return static_cast<int>(_lzcnt_u64(bitboard));
		}

		// returns the number of set bits in the bitboard
		force_inline int PopCount(const u64 bitboard)
		{
			return static_cast<int>(_mm_popcnt_u64(bitboard));
		}

		// returns the index of LS1B and clears that bit
		force_inline int PopBit(u64& bitboard)
		{
			int bit = BitScanForward(bitboard);
			bitboard &= bitboard - 1;
			return bit;
		}

		template<int dir>
		force_inline Bitboard StepOne(const Bitboard bitboard)
		{
			if constexpr (dir == NORTH) {
				return bitboard << 8;
			}
			else if (dir == SOUTH) {
				return bitboard >> 8;
			}
			else if (dir == EAST) {
				return (bitboard << 1) & C64(0xfefefefefefefefe);
			}
			else if (dir == WEST) {
				return (bitboard >> 1) & C64(0x7f7f7f7f7f7f7f7f);
			}
			else if (dir == NORTHEAST) {
				return (bitboard << 9) & C64(0xfefefefefefefefe);
			}
			else if (dir == NORTHWEST) {
				return (bitboard << 7) & C64(0x7f7f7f7f7f7f7f7f);
			}
			else if (dir == SOUTHEAST) {
				return (bitboard >> 7) & C64(0xfefefefefefefefe);
			}
			else if (dir == SOUTHWEST) {
				return (bitboard >> 9) & C64(0x7f7f7f7f7f7f7f7f);
			}
		}


		// returns a subset of bits. selection defines the order of bits starting from the LSB
		inline Bitboard BitSubset(Bitboard bitboard, const Bitboard selection)
		{
			Bitboard result = C64(0);

			// eg: if selection is 0x00..00101, the first and the third 1's in bitboard will be kept
			int num_bits = PopCount(bitboard);
			for (int i = 0; i < num_bits; i++) {
				int bit_index = PopBit(bitboard);
				if (BitIsSet(selection, i)) {
					SetBit(result, bit_index);
				}
			}

			return result;
		}

		
		inline void PrintBitboard(Bitboard bitboard)
		{
			if (bitboard == C64(0)) {
				printf("EMPTY BITBOARD\n");
				return;
			}

			int board[8][8]{};
			do {
				Square sq = BitScanForward(bitboard);
				board[GetRank(sq)][GetFile(sq)] = 1;
			} while (bitboard &= bitboard - 1);

			for (int r = RANK_EIGHT; r >= RANK_ONE; r--) {
				for (int f = FILE_A; f <= FILE_H; f++) {
					printf("%d ", board[r][f]);
				}
				putchar('\n');
			}
			putchar('\n');
		}
	};

} // namespace anka