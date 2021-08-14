#include "evaluation.hpp"
#include "boarddefs.hpp"
#include "movegen.hpp"

namespace anka {

	namespace {
		// Phase calculation piece weights for tapered eval.
		// Implementation is based on chessprogramming.org/Tapered_Eval
		constexpr int p_phase[8] = { 0, 0, 0, 1, 1, 2, 4, 0 };
		constexpr int opening_phase = p_phase[piece_type::PAWN] * 16 + p_phase[piece_type::KNIGHT] * 4
			+ p_phase[piece_type::BISHOP] * 4 + p_phase[piece_type::ROOK] * 4
			+ p_phase[piece_type::QUEEN] * 2;

		constexpr int piece_values_mg[8] = { 0, 0, 100, 320, 330, 500, 900, 0 };
		constexpr int piece_values_eg[8] = { 0, 0, 100, 320, 330, 500, 900, 0 };
		constexpr int mobility_scores[8] = { 0, 0, 2, 3, 2, 2, 2, 0 };
		constexpr int tempo_bonus = 5;
		constexpr int bishop_pair_bonus = 10;

		int PST_mg[2][8][64] {0};
		int PST_eg[2][8][64] {0};

	}

	void InitPST()
	{
		for (Side color = side::WHITE; color <= side::BLACK; color++) {
			for (Square sq = square::A1; sq <= square::H8; sq++) {
				Square pst_sq = (color == side::WHITE ? sq ^ 56 : sq);

				PST_mg[color][piece_type::PAWN][sq] = mg::pawns_pst[pst_sq];
				PST_mg[color][piece_type::KNIGHT][sq] = mg::knights_pst[pst_sq];
				PST_mg[color][piece_type::BISHOP][sq] = mg::bishops_pst[pst_sq];
				PST_mg[color][piece_type::ROOK][sq] = mg::rooks_pst[pst_sq];
				PST_mg[color][piece_type::QUEEN][sq] = mg::queens_pst[pst_sq];

				PST_eg[color][piece_type::PAWN][sq] = eg::pawns_pst[pst_sq];
				PST_eg[color][piece_type::KNIGHT][sq] = eg::knights_pst[pst_sq];
				PST_eg[color][piece_type::BISHOP][sq] = eg::bishops_pst[pst_sq];
				PST_eg[color][piece_type::ROOK][sq] = eg::rooks_pst[pst_sq];
				PST_eg[color][piece_type::QUEEN][sq] = eg::queens_pst[pst_sq];
			}
		}
	}


	int PawnMobility(Bitboard white_pawns, Bitboard black_pawns, Bitboard empty_squares)
	{
		using namespace piece_type;
		int score = 0;
		
		// white pawns
		Bitboard single_push_targets = bitboard::StepOne<direction::N>(white_pawns) & empty_squares;
		score += mobility_scores[PAWN] * bitboard::PopCount(single_push_targets);

		// opponent pawns
		single_push_targets = bitboard::StepOne<direction::S>(black_pawns) & empty_squares;
		score -= mobility_scores[PAWN] * bitboard::PopCount(single_push_targets);
		
		return score;
	}

	int PieceMobility(PieceType piece, Square sq, Bitboard occupation)
	{
		using namespace piece_type;
		Bitboard att = 0;
		Bitboard empty = ~occupation;

		switch (piece)
		{
		case KNIGHT:
			att = attacks::KnightAttacks(sq) & empty;
			break;
		case BISHOP:
			att = attacks::BishopAttacks(sq, occupation) & empty;
			break;
		case ROOK:
			att = attacks::RookAttacks(sq, occupation) & empty;
			break;
		case QUEEN: 
			att = attacks::QueenAttacks(sq, occupation) & empty;
			break;
		default:
			return 0;
		}

		return mobility_scores[piece] * bitboard::PopCount(att);
	}


	// Evaluates the position from side to play's perspective.
	// Returns a score in centipawns.
    int ClassicalEvaluation(const GameState& pos)
    {
		using namespace piece_type;

		// scores are evaluated from white's perspective
		// and flipped at the end if side to play is black
		int mg_score = 0;
		int eg_score = 0;
		int phase = opening_phase;
	
		Bitboard white_pawns = pos.Pieces<side::WHITE, PAWN>();
		Bitboard white_pieces = pos.Pieces<side::WHITE>() ^ white_pawns;
		Bitboard black_pawns = pos.Pieces<side::BLACK, PAWN>();
		Bitboard black_pieces = pos.Pieces<side::BLACK>() ^ black_pawns;
		Bitboard occ = pos.Occupancy();
		Bitboard empty = ~occ;

		// Pawn Mobility
		int pawn_mobility = PawnMobility(white_pawns, black_pawns, empty);
		mg_score += pawn_mobility;
		eg_score += pawn_mobility;


		while (white_pawns) {
			Square sq = bitboard::BitScanForward(white_pawns);
			mg_score += piece_values_mg[PAWN];
			eg_score += piece_values_eg[PAWN];

			mg_score += PST_mg[side::WHITE][PAWN][sq];
			eg_score += PST_eg[side::WHITE][PAWN][sq];

			white_pawns &= white_pawns - 1;
		}

		while (black_pawns) {
			Square sq = bitboard::BitScanForward(black_pawns);
			mg_score -= piece_values_mg[PAWN];
			eg_score -= piece_values_eg[PAWN];

			mg_score -= PST_mg[side::BLACK][PAWN][sq];
			eg_score -= PST_eg[side::BLACK][PAWN][sq];

			black_pawns &= black_pawns - 1;
		}

		// { 0, 0, 100, 300, 300, 500, 900, 0 };
		while (black_pieces) {
			Square sq = bitboard::BitScanForward(black_pieces);
			auto piece = pos.GetPiece(sq);
			phase -= p_phase[piece];
			
			mg_score -= piece_values_mg[piece];
			eg_score -= piece_values_eg[piece];

			mg_score -= PST_mg[side::BLACK][piece][sq];
			eg_score -= PST_eg[side::BLACK][piece][sq];

			int piece_mobility = PieceMobility(piece, sq, occ);
			mg_score -= piece_mobility;
			eg_score -= piece_mobility;

			black_pieces &= black_pieces - 1;		
		}

		while (white_pieces) {
			Square sq = bitboard::BitScanForward(white_pieces);
			auto piece = pos.GetPiece(sq);
			phase -= p_phase[piece];
			
			mg_score += piece_values_mg[piece];
			eg_score += piece_values_eg[piece];

			mg_score += PST_mg[side::WHITE][piece][sq];
			eg_score += PST_eg[side::WHITE][piece][sq];

			int piece_mobility = PieceMobility(piece, sq, occ);
			mg_score += piece_mobility;
			eg_score += piece_mobility;

			white_pieces &= white_pieces - 1;
		}

		// bishop pair bonus
		int num_white_bishops = bitboard::PopCount(pos.Pieces<side::WHITE, piece_type::BISHOP>());
		int num_black_bishops = bitboard::PopCount(pos.Pieces<side::BLACK, piece_type::BISHOP>());

		if (num_white_bishops > 1) {
			mg_score += bishop_pair_bonus;
			eg_score += bishop_pair_bonus;
		}

		if (num_black_bishops > 1) {
			mg_score -= bishop_pair_bonus;
			eg_score -= bishop_pair_bonus;
		}

		// Calculate game phase
		// (phase * 256 + (opening_phase / 2)) / opening_phase
		phase = ((phase << 8) + (opening_phase >> 1)) / opening_phase;

		// Interpolate the score between middle game and end game.
		// (phase = 0 at the beginning and phase = 256 at the endgame.)
		int score = ((mg_score * (256 - phase)) + (eg_score * phase)) >> 8;

		Side side = pos.SideToPlay();

		if (side == side::WHITE) {
			score += tempo_bonus;
		}
		if (side == side::BLACK) {
			score -= tempo_bonus;
			score = (-score);
		}

		return score;
    }
}