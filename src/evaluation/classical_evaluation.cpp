#include "evaluation.hpp"
#include "boarddefs.hpp"
#include "movegen.hpp"


namespace anka {
	namespace {
		// Phase calculation piece weights for tapered eval.
		// Implementation is based on chessprogramming.org/Tapered_Eval
		constexpr int p_phase[8] = { 0, 0, 0, 1, 1, 2, 4, 0 };
		constexpr int MAX_PHASE = p_phase[PAWN] * 16 + p_phase[KNIGHT] * 4
			+ p_phase[BISHOP] * 4 + p_phase[ROOK] * 4
			+ p_phase[QUEEN] * 2;


		EvalScore PieceMobility(PieceType piece, Square sq, Bitboard occupation, Bitboard ally_occupation)
		{
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
			score.phase_score[MG] = g_eval_params.mobility_weights[MG][piece] * num_attacks;
			score.phase_score[EG] = g_eval_params.mobility_weights[EG][piece] * num_attacks;
			return score;
		}

		template<Side color>
		void EvaluatePawns(Bitboard ally_pawns, Bitboard opponent_pawns, int *scores)
		{
			Bitboard pawns = ally_pawns;
			int mg = 0, eg = 0;

			while (pawns) {
				Square sq = bitboard::BitScanForward(pawns);

				mg += g_eval_params.PST_mg[color][PAWN][sq];
				eg += g_eval_params.PST_eg[color][PAWN][sq];

				//// check for passed pawns
				if ((attacks::PawnFrontSpan(sq, color) & opponent_pawns) == 0) {
					int rank = GetRank(sq);
					if constexpr (color == BLACK)
						rank = 7 - rank;
					mg += g_eval_params.passed_bonus[MG][rank];
					eg += g_eval_params.passed_bonus[EG][rank];
				}

				//// check for isolated pawns
				int file = GetFile(sq);
				if ((attacks::AdjacentFiles(file) & ally_pawns) == 0) {
					mg -= g_eval_params.isolated_penalty[MG][file];
					eg -= g_eval_params.isolated_penalty[EG][file];
				}

				pawns &= pawns - 1;
			}

			if constexpr (color == WHITE) {
				scores[0] += mg;
				scores[1] += eg;
			}
			else {
				scores[0] -= mg;
				scores[1] -= eg;
			}
		}


		template<Side color>
		int EvaluatePieces(const GameState& pos, int* scores)
		{

			int phase_change = 0;
			int mg = 0, eg = 0;
			Bitboard pieces = pos.Pieces<color>() ^ pos.Pieces<color, PAWN>();
			while (pieces) {
				Square sq = bitboard::BitScanForward(pieces);
				auto piece = pos.GetPiece(sq);
				phase_change += p_phase[piece];

				mg += g_eval_params.PST_mg[color][piece][sq];
				eg += g_eval_params.PST_eg[color][piece][sq];

				EvalScore piece_mobility = PieceMobility(piece, sq, pos.Occupancy(), pos.Pieces<color>());
				mg += piece_mobility.phase_score[MG];
				eg += piece_mobility.phase_score[EG];


				pieces &= pieces - 1;
			}

			if constexpr (color == WHITE) {
				scores[0] += mg;
				scores[1] += eg;
			}
			else {
				scores[0] -= mg;
				scores[1] -= eg;
			}

			return phase_change;
		}
	}

	// Evaluates the position from side to play's perspective.
	// Returns a score in centipawns.
    int GameState::ClassicalEvaluation() const
    {
		// Insufficient Material Draws: KvK, KNvK, KBvK
		if (m_piecesBB[PAWN] == 0) {
			int n_total = bitboard::PopCount(m_occupation);
			if (n_total == 2 || (n_total == 3 && (m_piecesBB[KNIGHT] | m_piecesBB[BISHOP])))
				return 0;
		}

		int scores[NUM_PHASES]{};
		int phase = MAX_PHASE;

		Bitboard w_pawns = Pieces<WHITE, PAWN>();
		Bitboard b_pawns = Pieces<BLACK, PAWN>();

		EvaluatePawns<WHITE>(w_pawns, b_pawns, scores);
		EvaluatePawns<BLACK>(b_pawns, w_pawns, scores);


		phase -= EvaluatePieces<WHITE>(*this, scores);
		phase -= EvaluatePieces<BLACK>(*this, scores);
	


		// bishop pair bonus
		int n_white_bishops = bitboard::PopCount(Pieces<WHITE, BISHOP>());
		int n_black_bishops= bitboard::PopCount(Pieces<BLACK, BISHOP>());
		if (n_white_bishops > 1) {
			scores[MG] += g_eval_params.bishop_pair_bonus[MG];
			scores[EG] += g_eval_params.bishop_pair_bonus[EG];
		}
		if (n_black_bishops > 1) {
			scores[MG] -= g_eval_params.bishop_pair_bonus[MG];
			scores[EG] -= g_eval_params.bishop_pair_bonus[EG];
		}



		// Map phase to [0-256]
		phase = ((phase << 8) + (MAX_PHASE >> 1)) / MAX_PHASE; 	// (phase * 256 + (MAX_PHASE / 2)) / MAX_PHASE

		// Interpolate the score between middle game and end game.
		// (phase = 0 at the beginning and phase = 256 at the endgame.)
		int result = ((scores[MG] * (256 - phase)) + (scores[EG] * phase)) >> 8;



		if (m_side == WHITE) {
			result += g_eval_params.tempo_bonus;

		}
		if (m_side == BLACK) {
			result -= g_eval_params.tempo_bonus;
			result = (-result);
		}


		return result;
    }


}