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

		// indexed by phase and piece type
		constexpr int piece_values[2][8] = { 
			{0, 0, 100, 320, 330, 500, 900, 0}, // midgame
			{0, 0, 100, 320, 330, 500, 900, 0}  // endgame
		};
		constexpr int mobility_weights[2][8] = { 
			{ 0, 0, 2, 3, 2, 2, 2, 0 }, // midgame
			{ 0, 0, 2, 3, 2, 2, 2, 0 }  // endgame
		};

		// indexed by phase and rank
		constexpr int passed_bonus_white[2][8] { 0, 10, 15, 20, 25, 30, 35, 0};
		constexpr int passed_bonus_black[2][8]{ 0, 35, 30, 25, 20, 15, 10 };

		// indexed by phase and file
		constexpr int isolated_penalty[2][8]{
			{ 10, 15, 20, 25, 25, 20, 15, 10 }, // midgame
			{ 10, 15, 20, 25, 25, 20, 15, 10 }  // endgame
		};

		// A B C D E F G H
		constexpr int tempo_bonus = 5;
		constexpr int bishop_pair_bonus = 10;


		int PST_mg[2][8][64] {0};
		int PST_eg[2][8][64] {0};

		struct EvalScore {
			int mid_game = 0;
			int end_game = 0;
		};

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

	EvalScore PawnMobility(Bitboard white_pawns, Bitboard black_pawns, Bitboard empty_squares)
	{
		using namespace piece_type;
		EvalScore score;
		
		// white pawns
		Bitboard single_push_targets = bitboard::StepOne<NORTH>(white_pawns) & empty_squares;
		score.mid_game += mobility_weights[MIDGAME][PAWN] * bitboard::PopCount(single_push_targets);
		score.end_game += mobility_weights[ENDGAME][PAWN] * bitboard::PopCount(single_push_targets);

		// opponent pawns
		single_push_targets = bitboard::StepOne<SOUTH>(black_pawns) & empty_squares;
		score.mid_game -= mobility_weights[MIDGAME][PAWN] * bitboard::PopCount(single_push_targets);
		score.end_game -= mobility_weights[ENDGAME][PAWN] * bitboard::PopCount(single_push_targets);
		
		return score;
	}

	EvalScore PieceMobility(PieceType piece, Square sq, Bitboard occupation, Bitboard ally_occupation)
	{
		using namespace piece_type;
		EvalScore score;
		Bitboard att = 0;
		Bitboard legal_squares = ~ally_occupation; // pseudo legal. empty and opponent squares

		switch (piece)
		{
		case KNIGHT:
			att = attacks::KnightAttacks(sq) & legal_squares;
			break;
		case BISHOP:
			att = attacks::BishopAttacks(sq, occupation) & legal_squares;
			break;
		case ROOK:
			att = attacks::RookAttacks(sq, occupation) & legal_squares;
			break;
		case QUEEN: 
			att = attacks::QueenAttacks(sq, occupation) & legal_squares;
			break;
		default:
			return score;
		}

		int num_attacks = bitboard::PopCount(att);
		score.mid_game = mobility_weights[MIDGAME][piece] * num_attacks;
		score.end_game = mobility_weights[ENDGAME][piece] * num_attacks;
		return score;
	}


	// Evaluates the position from side to play's perspective.
	// Returns a score in centipawns.
    int ClassicalEvaluation(const GameState& pos)
    {
		using namespace piece_type;

		// scores are evaluated from white's perspective
		// and flipped at the end if side to play is black
		EvalScore score;
		int phase = opening_phase;
	
		Bitboard white_pawns = pos.Pieces<side::WHITE, PAWN>();
		Bitboard white_pieces = pos.Pieces<side::WHITE>() ^ white_pawns;
		Bitboard black_pawns = pos.Pieces<side::BLACK, PAWN>();
		Bitboard black_pieces = pos.Pieces<side::BLACK>() ^ black_pawns;
		Bitboard occ = pos.Occupancy();
		Bitboard empty = ~occ;

		// Pawn Mobility
		EvalScore pawn_mobility = PawnMobility(white_pawns, black_pawns, empty);
		score.mid_game += pawn_mobility.mid_game;
		score.end_game += pawn_mobility.end_game;

		// isolated pawn: a pawn with no friendly pawns on adjacent files
		// passed pawn: a pawn with no opponent pawns in front or adjacent files

		while (white_pawns) {
			Square sq = bitboard::BitScanForward(white_pawns);
			score.mid_game += piece_values[MIDGAME][PAWN];
			score.end_game += piece_values[ENDGAME][PAWN];

			score.mid_game += PST_mg[side::WHITE][PAWN][sq];
			score.end_game += PST_eg[side::WHITE][PAWN][sq];

			//// check if passed pawn
			if ((attacks::PawnFrontSpan(sq, side::WHITE) & pos.Pieces<side::BLACK, PAWN>()) == 0) {
				int rank = square::GetRank(sq);
				score.mid_game += passed_bonus_white[MIDGAME][rank];
				score.end_game += passed_bonus_white[ENDGAME][rank];
			}

			//// check if isolated pawn
			int file = square::GetFile(sq);
			if ((attacks::AdjacentFiles(file) & pos.Pieces<side::WHITE, PAWN>()) == 0) {
				score.mid_game -= isolated_penalty[MIDGAME][file];
				score.end_game -= isolated_penalty[ENDGAME][file];
			}

			white_pawns &= white_pawns - 1;
		}

		while (black_pawns) {
			Square sq = bitboard::BitScanForward(black_pawns);
			score.mid_game -= piece_values[MIDGAME][PAWN];
			score.end_game -= piece_values[ENDGAME][PAWN];

			score.mid_game -= PST_mg[side::BLACK][PAWN][sq];
			score.end_game -= PST_eg[side::BLACK][PAWN][sq];

			//// check if passed pawn
			if ((attacks::PawnFrontSpan(sq, side::BLACK) & pos.Pieces<side::WHITE, PAWN>()) == 0) {
				int rank = square::GetRank(sq);
				score.mid_game -= passed_bonus_black[MIDGAME][rank];
				score.end_game -= passed_bonus_black[ENDGAME][rank];
			}

			//// check if isolated pawn
			int file = square::GetFile(sq);
			if ((attacks::AdjacentFiles(file) & pos.Pieces<side::WHITE, PAWN>()) == 0) {
				score.mid_game += isolated_penalty[MIDGAME][file];
				score.end_game += isolated_penalty[ENDGAME][file];
			}

			black_pawns &= black_pawns - 1;
		}

		// { 0, 0, 100, 300, 300, 500, 900, 0 };
		while (black_pieces) {
			Square sq = bitboard::BitScanForward(black_pieces);
			auto piece = pos.GetPiece(sq);
			phase -= p_phase[piece];
			
			score.mid_game -= piece_values[MIDGAME][piece];
			score.end_game -= piece_values[ENDGAME][piece];

			score.mid_game -= PST_mg[side::BLACK][piece][sq];
			score.end_game -= PST_eg[side::BLACK][piece][sq];

			EvalScore piece_mobility = PieceMobility(piece, sq, occ, pos.Pieces<side::BLACK>());
			score.mid_game -= piece_mobility.mid_game;
			score.end_game -= piece_mobility.end_game;

			black_pieces &= black_pieces - 1;		
		}

		while (white_pieces) {
			Square sq = bitboard::BitScanForward(white_pieces);
			auto piece = pos.GetPiece(sq);
			phase -= p_phase[piece];
			
			score.mid_game += piece_values[MIDGAME][piece];
			score.end_game += piece_values[ENDGAME][piece];

			score.mid_game += PST_mg[side::WHITE][piece][sq];
			score.end_game += PST_eg[side::WHITE][piece][sq];

			EvalScore piece_mobility = PieceMobility(piece, sq, occ, pos.Pieces<side::WHITE>());
			score.mid_game += piece_mobility.mid_game;
			score.end_game += piece_mobility.end_game;

			white_pieces &= white_pieces - 1;
		}

		// bishop pair bonus
		int num_white_bishops = bitboard::PopCount(pos.Pieces<side::WHITE, piece_type::BISHOP>());
		int num_black_bishops = bitboard::PopCount(pos.Pieces<side::BLACK, piece_type::BISHOP>());

		if (num_white_bishops > 1) {
			score.mid_game += bishop_pair_bonus;
			score.end_game += bishop_pair_bonus;
		}

		if (num_black_bishops > 1) {
			score.mid_game -= bishop_pair_bonus;
			score.end_game -= bishop_pair_bonus;
		}

		// Calculate game phase
		// (phase * 256 + (opening_phase / 2)) / opening_phase
		phase = ((phase << 8) + (opening_phase >> 1)) / opening_phase;

		// Interpolate the score between middle game and end game.
		// (phase = 0 at the beginning and phase = 256 at the endgame.)
		int result = ((score.mid_game * (256 - phase)) + (score.end_game * phase)) >> 8;

		Side color = pos.SideToPlay();

		if (color == side::WHITE) {
			result += tempo_bonus;
		}
		if (color == side::BLACK) {
			result -= tempo_bonus;
			result = (-result);
		}

		return result;
    }
}