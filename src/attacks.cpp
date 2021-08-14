#include "attacks.hpp"
#include "boarddefs.hpp"
#include <vector>


namespace anka {
	namespace attacks {

		Bitboard _slider_attacks[115084];
		Bitboard _knight_attacks[64];
		Bitboard _king_attacks[64];
		Bitboard _pawn_attacks[2][64];
		Bitboard _pawn_front_spans[2][64];
		Bitboard _in_between[64][64];
		Bitboard _adjacent_files[8];


		template <int dir>
		static Bitboard Fill(Bitboard b)
		{
			static_assert(dir == NORTH || dir == SOUTH, "Invalid fill direction");
			
			if constexpr (dir == NORTH) {
				b |= (b << 8);
				b |= (b << 16);
				b |= (b << 32);
			}
			else if constexpr (dir == SOUTH) {
				b |= (b >> 8);
				b |= (b >> 16);
				b |= (b >> 32);
			}

			return b;
		}

		Bitboard RookAttacksSlow(Square sq, Bitboard relevant_occ)
		{
			Bitboard attack_map{};
			int origin = square::Square64To120(sq);
			int dirs[] = { 10, -1, -10, 1 };

			for (auto dir : dirs) {
				int target = origin + dir;
				while (!square::IsOffBoard120(target)) {
					Square target64 = square::Square120To64(target);
					bitboard::SetBit(attack_map, target64);
					if (bitboard::BitIsSet(relevant_occ, target64))
						break;

					target += dir;
				}
			}

			return attack_map;
		}

		Bitboard BishopAttacksSlow(Square sq, Bitboard relevant_occ)
		{
			Bitboard attack_map = C64(0);
			int origin = square::Square64To120(sq);
			int dirs[] = { 11, 9, -11,-9 };

			for (auto dir : dirs) {
				int target = origin + dir;
				while (!square::IsOffBoard120(target)) {
					Square target64 = square::Square120To64(target);
					bitboard::SetBit(attack_map, target64);
					if (bitboard::BitIsSet(relevant_occ, target64))
						break;

					target += dir;
				}
			}

			return attack_map;
		}

		// generate all possible rook occupancy maps for a given square
		static std::vector<Bitboard> GenerateRookOccupancy(Square sq)
		{
			std::vector<Bitboard> result;
			Bitboard attacks = magics::rook_magics[sq].mask;
			int num_bits = bitboard::PopCount(attacks);

			Bitboard occupancy = C64(0);
			for (int i = 0; i < (2 << num_bits); i++) {
				occupancy = bitboard::BitSubset(attacks, i);
				result.push_back(occupancy);
			}

			return result;
		}

		// generate all possible bishop occupancy maps for a given square
		static std::vector<Bitboard> GenerateBishopOccupancy(Square sq)
		{
			std::vector<Bitboard> result;
			Bitboard attacks = magics::bishop_magics[sq].mask;
			int num_bits = bitboard::PopCount(attacks);

			Bitboard occupancy = C64(0);
			for (int i = 0; i < (2 << num_bits); i++) {
				occupancy = bitboard::BitSubset(attacks, i);
				result.push_back(occupancy);
			}

			return result;
		}

		static void InitSliderAttacks()
		{
			// init bishop attack tables
			for (int sq = 0; sq < 64; sq++) {
				u64 mask = magics::bishop_magics[sq].mask;
				u64 magic = magics::bishop_magics[sq].magic_factor;
				int table_offset = magics::bishop_magics[sq].table_offset;

				auto occlist = GenerateBishopOccupancy(sq);
				for (u64 occ : occlist) {
					Bitboard a = occ;
					
					u64 blockers = occ & mask;
					int index = (blockers * magic) >> (64 - 9);
					_slider_attacks[table_offset + index] = BishopAttacksSlow(sq, blockers);
				}

			}

			// init rook attack tables
			for (int sq = 0; sq < 64; sq++) {
				u64 mask = magics::rook_magics[sq].mask;
				u64 magic = magics::rook_magics[sq].magic_factor;
				int table_offset = magics::rook_magics[sq].table_offset;

				auto occlist = GenerateRookOccupancy(sq);
				for (u64 occ : occlist) {
					u64 blockers = occ & mask;
					int index = (blockers * magic) >> (64 - 12);
					_slider_attacks[table_offset + index] = RookAttacksSlow(sq, blockers);
				}

			}
		}

		static void InitKnightAttacks()
		{
			int knight_dirs[] = { -8, -19, -21, -12, 8, 19, 21, 12 };
			
			for (int sq = 0; sq < 64; sq++) {
				Bitboard attack_map{};
				int origin = square::Square64To120(sq);

				for (auto dir : knight_dirs) {
					int target = origin + dir;
					if (square::IsOffBoard120(target))
						continue;
					Square target64 = square::Square120To64(target);
					bitboard::SetBit(attack_map, target64);
				}
				_knight_attacks[sq] = attack_map;
			}
		}

		static void InitKingAttacks()
		{
			int king_dirs[] = { 10, -1, -10, 1, 11, 9, -11, -9 };

			for (int sq = 0; sq < 64; sq++) {
				Bitboard attack_map{};
				int origin = square::Square64To120(sq);

				for (auto dir : king_dirs) {
					int target = origin + dir;
					if (square::IsOffBoard120(target))
						continue;
					Square target64 = square::Square120To64(target);
					bitboard::SetBit(attack_map, target64);
				}
				_king_attacks[sq] = attack_map;
			}
		}

		static void InitPawnAttacks()
		{
			for (int sq = 0; sq < 64; sq++) {
				// White pawn attack
				u64 attack_map = 0;
				int origin = square::Square64To120(sq);
				int target_1 = origin + 9;
				int target_2 = origin + 11;

				if (!square::IsOffBoard120(target_1)) {
					bitboard::SetBit(attack_map, square::Square120To64(target_1));
				}

				if (!square::IsOffBoard120(target_2)) {
					bitboard::SetBit(attack_map, square::Square120To64(target_2));
				}
				_pawn_attacks[side::WHITE][sq] = attack_map;
			}

			for (int sq = 0; sq < 64; sq++) {
				// Black pawn attack
				u64 attack_map = 0;
				int origin = square::Square64To120(sq);
				int target_1 = origin - 9;
				int target_2 = origin - 11;

				if (!square::IsOffBoard120(target_1)) {
					bitboard::SetBit(attack_map, square::Square120To64(target_1));
				}

				if (!square::IsOffBoard120(target_2)) {
					bitboard::SetBit(attack_map, square::Square120To64(target_2));
				}
				_pawn_attacks[side::BLACK][sq] = attack_map;
			}
		}

		static void InitPawnFrontSpans()
		{
			for (Square sq = square::A1; sq <= square::H8; sq++) {
				Bitboard b = 0;
				bitboard::SetBit(b, sq);
				b |= bitboard::StepOne<WEST>(b);
				b |= bitboard::StepOne<EAST>(b);
				_pawn_front_spans[side::WHITE][sq] = Fill<NORTH>(bitboard::StepOne<NORTH>(b));
				_pawn_front_spans[side::BLACK][sq] = Fill<SOUTH>(bitboard::StepOne<SOUTH>(b));
			}
		}

		static void InitAdjacentFiles()
		{
			for (Square sq = square::A1; sq <= square::H1; sq++) {
				Bitboard b = 0;
				bitboard::SetBit(b, sq);
				b |= bitboard::StepOne<WEST>(b);
				b |= bitboard::StepOne<EAST>(b);
				bitboard::ClearBit(b, sq);
				_adjacent_files[sq] = Fill<NORTH>(b);
			}

		}

		static void InitInBetween()
		{
			for (int i = 0; i < 64; i++) {
				for (int j = 0; j < 64; j++) {
					_in_between[i][j] = C64(0);
				}
			}

			
			for (int from = 0; from < 64; from++) {
				for (int to = from + 1; to < 64; to++) {
					Bitboard in_between = C64(0);
					if (square::GetRank(from) == square::GetRank(to)) {
						//std::cout << "Same rank: f: " << from << " t: " << to << std::endl;
						int num_inbetween = to - from - 1;
						while (num_inbetween) {
							bitboard::SetBit(in_between, to - num_inbetween);
							num_inbetween--;
						}
					}
					else if (square::GetFile(from) == square::GetFile(to)) {
						//std::cout << "Same file: f: " << from << " t: " << to << std::endl;
						int num_inbetween = (to - from) / 8 - 1;
						while (num_inbetween) {
							bitboard::SetBit(in_between, to - num_inbetween*8);
							num_inbetween--;
						}
					}
					else if ((to - from) % 9 == 0) {
						//std::cout << "Same diagonal: f: " << from << " t: " << to << std::endl;
						int num_inbetween = (to - from) / 9 - 1;
						while (num_inbetween) {
							bitboard::SetBit(in_between, to - num_inbetween * 9);
							num_inbetween--;
						}
					}
					else if ((to - from) % 7 == 0) {
						//std::cout << "Same diagonal: f: " << from << " t: " << to << std::endl;
						int num_inbetween = (to - from) / 7 - 1;
						while (num_inbetween) {
							bitboard::SetBit(in_between, to - num_inbetween * 7);
							num_inbetween--;
						}
					}

					_in_between[from][to] = in_between;
					_in_between[to][from] = in_between;
				}
			}
		}

		void InitAttacks()
		{
			InitSliderAttacks();
			InitKnightAttacks();
			InitKingAttacks();
			InitPawnAttacks();
			InitPawnFrontSpans();
			InitAdjacentFiles();
			InitInBetween();
		}



	} // namespace attacks
} // namespace anka
