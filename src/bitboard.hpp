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
			int bit_index = BitScanForward(bitboard);
			ClearBit(bitboard, bit_index);
			return bit_index;
		}

		//// return vertically flipped bitboard
		//force_inline Bitboard Flip() const
		//{
		//	return _byteswap_uint64(bitboard);
		//}

		template<int dir>
		force_inline Bitboard StepOne(const Bitboard bitboard)
		{
			if constexpr (dir == direction::N) {
				return bitboard << 8;
			}
			else if (dir == direction::S) {
				return bitboard >> 8;
			}
			else if (dir == direction::E) {
				return (bitboard << 1) & C64(0xfefefefefefefefe);
			}
			else if (dir == direction::W) {
				return (bitboard >> 1) & C64(0x7f7f7f7f7f7f7f7f);
			}
			else if (dir == direction::NE) {
				return (bitboard << 9) & C64(0xfefefefefefefefe);
			}
			else if (dir == direction::NW) {
				return (bitboard << 7) & C64(0x7f7f7f7f7f7f7f7f);
			}
			else if (dir == direction::SE) {
				return (bitboard >> 7) & C64(0xfefefefefefefefe);
			}
			else if (dir == direction::SW) {
				return (bitboard >> 9) & C64(0x7f7f7f7f7f7f7f7f);
			}
		}


		// returns a subset of bits. selection defines the order of bits starting from the LSB
		inline Bitboard BitSubset(Bitboard bitboard, const Bitboard selection)
		{
			Bitboard result = C64(0);

			// eg: if selection is 0x00..00101, the first and the third 1's in bitboard will be kept
			int num_bitboard = PopCount(bitboard);
			for (int i = 0; i < num_bitboard; i++) {
				int bit_index = PopBit(bitboard);
				if (BitIsSet(selection, i)) {
					SetBit(result, bit_index);
				}
			}

			return result;
		}

		
		inline void Print(Bitboard bitboard)
		{
			if (bitboard == C64(0)) {
				printf("EMPTY BITBOARD\n");
				return;
			}

			int board[8][8]{};
			do {
				Square sq = BitScanForward(bitboard);
				board[square::GetRank(sq)][square::GetFile(sq)] = 1;
			} while (bitboard &= bitboard - 1);

			for (int r = rank::EIGHT; r >= rank::ONE; r--) {
				for (int f = file::A; f <= file::H; f++) {
					printf("%d ", board[r][f]);
				}
				putchar('\n');
			}
			putchar('\n');
		}
	};

} // namespace anka