#include "hash.hpp"
#include "gamestate.hpp"

namespace anka {
	namespace zobrist_keys {
		u64 piece_keys[2][6][64]{};
		u64 castle_keys[16]{};
		u64 side_key{};
		u64 ep_keys[65]{}; // includes NO_SQUARE(64)
	} // namespace zobrist_keys

	void InitZobristKeys(RNG& rng)
	{
		for (int s = 0; s < 2; s++) {
			for (int t = 0; t < 6; t++) {
				for (int sq = 0; sq < 64; sq++) {
					zobrist_keys::piece_keys[s][t][sq] = rng.rand64();
				}
			}
		}

		for (int i = 0; i < 65; i++) {
			zobrist_keys::ep_keys[i] = rng.rand64();
		}

		for (int i = 0; i < 16; i++) {
			zobrist_keys::castle_keys[i] = rng.rand64();
		}


		zobrist_keys::side_key = rng.rand64();
	}


	u64 GameState::CalculateKey()
	{
		u64 key = C64(0);
	
		for (Side color = WHITE; color <= BLACK; color++) {
			for (PieceType t = PAWN; t <= KING; t++) {
				Bitboard pieces = m_piecesBB[t] & m_piecesBB[color];
				while (pieces) {
					Square sq = bitboard::BitScanForward(pieces);
					key ^= zobrist_keys::piece_keys[color][t-2][sq];
					pieces &= pieces - 1;
				};
			}
		}

		if (m_side == BLACK) {
			key ^= zobrist_keys::side_key;
		}

		key ^= zobrist_keys::castle_keys[m_castling_rights];
		key ^= zobrist_keys::ep_keys[m_ep_target];
		
		return key;
	}
}