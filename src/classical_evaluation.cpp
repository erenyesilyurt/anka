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
		constexpr int piece_values[NUM_PHASES][8] = { 
			{0, 0, 100, 320, 330, 500, 900, 0}, // midgame
			{0, 0, 100, 320, 330, 500, 900, 0}  // endgame
		};
		constexpr int mobility_weights[NUM_PHASES][8] = { 
			{ 0, 0, 2, 3, 2, 2, 2, 0 }, // midgame
			{ 0, 0, 2, 3, 2, 2, 2, 0 }  // endgame
		};

		// indexed by phase and rank
		constexpr int passed_bonus[NUM_PHASES][8]{ 
			{ 0, 10, 15, 20, 25, 30, 35, 0},
			{ 0, 10, 15, 20, 25, 30, 35, 0} 
		};

		// indexed by phase and file
		constexpr int isolated_penalty[NUM_PHASES][8]{
			{ 10, 15, 20, 25, 25, 20, 15, 10 }, // midgame
			{ 10, 15, 20, 25, 25, 20, 15, 10 }  // endgame
		};

		constexpr int bishop_pair_bonus[NUM_PHASES] = { 10, 10 };
		constexpr int tempo_bonus = 5;
		
		int PST_mg[2][8][64] {0};
		int PST_eg[2][8][64] {0};

		struct EvalScore {
			int mid_game = 0;
			int end_game = 0;
		};

	}

	#ifdef EVAL_DEBUG
	EvalInfo eval_info;
	#endif

	void InitPST()
	{
		for (Side color = WHITE; color < NUM_SIDES; color++) {
			for (Square sq = square::A1; sq <= square::H8; sq++) {
				Square pst_sq = (color == WHITE ? sq ^ 56 : sq);

				PST_mg[color][piece_type::PAWN][sq] = mg::pawns_pst[pst_sq];
				PST_mg[color][piece_type::KNIGHT][sq] = mg::knights_pst[pst_sq];
				PST_mg[color][piece_type::BISHOP][sq] = mg::bishops_pst[pst_sq];
				PST_mg[color][piece_type::ROOK][sq] = mg::rooks_pst[pst_sq];
				PST_mg[color][piece_type::QUEEN][sq] = mg::queens_pst[pst_sq];
				PST_mg[color][piece_type::KING][sq] = mg::king_pst[pst_sq];

				PST_eg[color][piece_type::PAWN][sq] = eg::pawns_pst[pst_sq];
				PST_eg[color][piece_type::KNIGHT][sq] = eg::knights_pst[pst_sq];
				PST_eg[color][piece_type::BISHOP][sq] = eg::bishops_pst[pst_sq];
				PST_eg[color][piece_type::ROOK][sq] = eg::rooks_pst[pst_sq];
				PST_eg[color][piece_type::QUEEN][sq] = eg::queens_pst[pst_sq];
				PST_eg[color][piece_type::KING][sq] = eg::king_pst[pst_sq];
			}
		}
	}

	EvalScore PawnMobility(Bitboard white_pawns, Bitboard black_pawns, Bitboard empty_squares)
	{
		using namespace piece_type;
		EvalScore score;
		
		// white pawns
		Bitboard single_push_targets = bitboard::StepOne<NORTH>(white_pawns) & empty_squares;
		int num_targets = bitboard::PopCount(single_push_targets);
		score.mid_game += mobility_weights[MIDGAME][PAWN] * num_targets;
		score.end_game += mobility_weights[ENDGAME][PAWN] * num_targets;

		EVAL_INFO("mobility", WHITE, score.mid_game, score.end_game);

		// black pawns
		single_push_targets = bitboard::StepOne<SOUTH>(black_pawns) & empty_squares;
		num_targets = bitboard::PopCount(single_push_targets);
		score.mid_game -= mobility_weights[MIDGAME][PAWN] * num_targets;
		score.end_game -= mobility_weights[ENDGAME][PAWN] * num_targets;
		
		EVAL_INFO("mobility", BLACK, mobility_weights[MIDGAME][PAWN] * num_targets, mobility_weights[ENDGAME][PAWN] * num_targets);


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



	//  TODO: test on symmetrical positions by switching sides
	// Evaluates the position from side to play's perspective.
	// Returns a score in centipawns.
    int ClassicalEvaluation(const GameState& pos)
    {
		using namespace piece_type;

		#ifdef EVAL_DEBUG
		eval_info = {};
		#endif // EVAL_DEBUG

		// scores are evaluated from white's perspective
		// and flipped at the end if side to play is black
		EvalScore score;
		int phase = opening_phase;
	
		Bitboard white_pawns = pos.Pieces<WHITE, PAWN>();
		Bitboard white_pieces = pos.Pieces<WHITE>() ^ white_pawns;
		Bitboard black_pawns = pos.Pieces<BLACK, PAWN>();
		Bitboard black_pieces = pos.Pieces<BLACK>() ^ black_pawns;
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

			EVAL_INFO("material", WHITE, piece_values[MIDGAME][PAWN], piece_values[ENDGAME][PAWN]);

			score.mid_game += PST_mg[WHITE][PAWN][sq];
			score.end_game += PST_eg[WHITE][PAWN][sq];

			EVAL_INFO("pst", WHITE, PST_mg[WHITE][PAWN][sq], PST_eg[WHITE][PAWN][sq]);

			//// check if passed pawn
			if ((attacks::PawnFrontSpan(sq, WHITE) & pos.Pieces<BLACK, PAWN>()) == 0) {
				int rank = square::GetRank(sq);
				score.mid_game += passed_bonus[MIDGAME][rank];
				score.end_game += passed_bonus[ENDGAME][rank];

				EVAL_INFO("passed", WHITE, passed_bonus[MIDGAME][rank], passed_bonus[ENDGAME][rank]);
			}

			//// check if isolated pawn
			int file = square::GetFile(sq);
			if ((attacks::AdjacentFiles(file) & pos.Pieces<WHITE, PAWN>()) == 0) {
				score.mid_game -= isolated_penalty[MIDGAME][file];
				score.end_game -= isolated_penalty[ENDGAME][file];

				EVAL_INFO("isolated", WHITE, -isolated_penalty[MIDGAME][file], -isolated_penalty[ENDGAME][file]);
			}

			white_pawns &= white_pawns - 1;
		}

		while (black_pawns) {
			Square sq = bitboard::BitScanForward(black_pawns);
			score.mid_game -= piece_values[MIDGAME][PAWN];
			score.end_game -= piece_values[ENDGAME][PAWN];

			EVAL_INFO("material", BLACK, piece_values[MIDGAME][PAWN], piece_values[ENDGAME][PAWN]);

			score.mid_game -= PST_mg[BLACK][PAWN][sq];
			score.end_game -= PST_eg[BLACK][PAWN][sq];

			EVAL_INFO("pst", BLACK, PST_mg[BLACK][PAWN][sq], PST_eg[BLACK][PAWN][sq]);

			//// check if passed pawn
			if ((attacks::PawnFrontSpan(sq, BLACK) & pos.Pieces<WHITE, PAWN>()) == 0) {
				int rank = 7 - square::GetRank(sq);
				score.mid_game -= passed_bonus[MIDGAME][rank];
				score.end_game -= passed_bonus[ENDGAME][rank];

				EVAL_INFO("passed", BLACK, passed_bonus[MIDGAME][rank], passed_bonus[ENDGAME][rank]);
			}

			//// check if isolated pawn
			int file = square::GetFile(sq);
			if ((attacks::AdjacentFiles(file) & pos.Pieces<BLACK, PAWN>()) == 0) {
				score.mid_game += isolated_penalty[MIDGAME][file];
				score.end_game += isolated_penalty[ENDGAME][file];

				EVAL_INFO("isolated", BLACK, -isolated_penalty[MIDGAME][file], -isolated_penalty[ENDGAME][file]);
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

			EVAL_INFO("material", BLACK, piece_values[MIDGAME][piece], piece_values[ENDGAME][piece]);

			score.mid_game -= PST_mg[BLACK][piece][sq];
			score.end_game -= PST_eg[BLACK][piece][sq];

			EVAL_INFO("pst", BLACK, PST_mg[BLACK][piece][sq], PST_eg[BLACK][piece][sq]);

			EvalScore piece_mobility = PieceMobility(piece, sq, occ, pos.Pieces<BLACK>());
			score.mid_game -= piece_mobility.mid_game;
			score.end_game -= piece_mobility.end_game;

			EVAL_INFO("mobility", BLACK, piece_mobility.mid_game, piece_mobility.end_game);

			black_pieces &= black_pieces - 1;		
		}

		while (white_pieces) {
			Square sq = bitboard::BitScanForward(white_pieces);
			auto piece = pos.GetPiece(sq);
			phase -= p_phase[piece];
			
			score.mid_game += piece_values[MIDGAME][piece];
			score.end_game += piece_values[ENDGAME][piece];

			EVAL_INFO("material", WHITE, piece_values[MIDGAME][piece], piece_values[ENDGAME][piece]);

			score.mid_game += PST_mg[WHITE][piece][sq];
			score.end_game += PST_eg[WHITE][piece][sq];

			EVAL_INFO("pst", WHITE, PST_mg[WHITE][piece][sq], PST_eg[WHITE][piece][sq]);

			EvalScore piece_mobility = PieceMobility(piece, sq, occ, pos.Pieces<WHITE>());
			score.mid_game += piece_mobility.mid_game;
			score.end_game += piece_mobility.end_game;

			EVAL_INFO("mobility", WHITE, piece_mobility.mid_game, piece_mobility.end_game);

			white_pieces &= white_pieces - 1;
		}

		// bishop pair bonus
		int num_white_bishops = bitboard::PopCount(pos.Pieces<WHITE, piece_type::BISHOP>());
		int num_black_bishops = bitboard::PopCount(pos.Pieces<BLACK, piece_type::BISHOP>());

		if (num_white_bishops > 1) {
			score.mid_game += bishop_pair_bonus[MIDGAME];
			score.end_game += bishop_pair_bonus[ENDGAME];

			EVAL_INFO("bishop_pair", WHITE, bishop_pair_bonus[MIDGAME], bishop_pair_bonus[ENDGAME]);
		}

		if (num_black_bishops > 1) {
			score.mid_game -= bishop_pair_bonus[MIDGAME];
			score.end_game -= bishop_pair_bonus[ENDGAME];

			EVAL_INFO("bishop_pair", BLACK, bishop_pair_bonus[MIDGAME], bishop_pair_bonus[ENDGAME]);
		}

		// Calculate game phase
		// (phase * 256 + (opening_phase / 2)) / opening_phase
		phase = ((phase << 8) + (opening_phase >> 1)) / opening_phase;
		EVAL_INFO("phase", NOSIDE, phase, 0);

		// Interpolate the score between middle game and end game.
		// (phase = 0 at the beginning and phase = 256 at the endgame.)
		int result = ((score.mid_game * (256 - phase)) + (score.end_game * phase)) >> 8;

		Side color = pos.SideToPlay();
		EVAL_INFO("tempo", color, tempo_bonus, 0);
		if (color == WHITE) {
			result += tempo_bonus;

		}
		if (color == BLACK) {
			result -= tempo_bonus;
			result = (-result);
		}

		return result;
    }


}