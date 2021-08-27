#include "evaluation.hpp"
#include "boarddefs.hpp"
#include "movegen.hpp"


namespace anka {
	namespace {
		// Phase calculation piece weights for tapered eval.
		// Implementation is based on chessprogramming.org/Tapered_Eval
		constexpr int p_phase[8] = { 0, 0, 0, 1, 1, 2, 4, 0 };
		constexpr int MAX_PHASE = p_phase[piece_type::PAWN] * 16 + p_phase[piece_type::KNIGHT] * 4
			+ p_phase[piece_type::BISHOP] * 4 + p_phase[piece_type::ROOK] * 4
			+ p_phase[piece_type::QUEEN] * 2;


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
			score.phase_score[MIDGAME] = g_eval_params.mobility_weights[MIDGAME][piece] * num_attacks;
			score.phase_score[ENDGAME] = g_eval_params.mobility_weights[ENDGAME][piece] * num_attacks;
			return score;
		}

		template<Side color>
		void EvaluatePawns(const GameState& pos)
		{
			using namespace piece_type;
			constexpr Side opposite_color = color ^ 1;
			Bitboard ally_pawns = pos.Pieces<color, PAWN>();
			Bitboard opponent_pawns = pos.Pieces<opposite_color, PAWN>();

			Bitboard pawns = ally_pawns;
			while (pawns) {
				Square sq = bitboard::BitScanForward(pawns);
				g_eval_data.material[MIDGAME][color] += g_eval_params.piece_values[MIDGAME][PAWN];
				g_eval_data.material[ENDGAME][color] += g_eval_params.piece_values[ENDGAME][PAWN];
				g_eval_data.pst[MIDGAME][color] += g_eval_params.PST_mg[color][PAWN][sq];
				g_eval_data.pst[ENDGAME][color] += g_eval_params.PST_eg[color][PAWN][sq];


				//// check if passed pawn
				if ((attacks::PawnFrontSpan(sq, color) & opponent_pawns) == 0) {
					int rank = square::GetRank(sq);
					if constexpr (color == BLACK)
						rank = 7 - rank;
					g_eval_data.pawn_structure[MIDGAME][color] += g_eval_params.passed_bonus[MIDGAME][rank];
					g_eval_data.pawn_structure[ENDGAME][color] += g_eval_params.passed_bonus[ENDGAME][rank];

				}

				//// check if isolated pawn
				int file = square::GetFile(sq);
				if ((attacks::AdjacentFiles(file) & ally_pawns) == 0) {
					g_eval_data.pawn_structure[MIDGAME][color] -= g_eval_params.isolated_penalty[MIDGAME][file];
					g_eval_data.pawn_structure[ENDGAME][color] -= g_eval_params.isolated_penalty[ENDGAME][file];
				}

				pawns &= pawns - 1;
			}
		}


		template<Side color>
		int EvaluatePieces(const GameState& pos)
		{
			using namespace piece_type;

			int phase_change = 0;
			Bitboard pieces = pos.Pieces<color>() ^ pos.Pieces<color, PAWN>();
			while (pieces) {
				Square sq = bitboard::BitScanForward(pieces);
				auto piece = pos.GetPiece(sq);
				phase_change += p_phase[piece];

				g_eval_data.material[MIDGAME][color] += g_eval_params.piece_values[MIDGAME][piece];
				g_eval_data.material[ENDGAME][color] += g_eval_params.piece_values[ENDGAME][piece];

				g_eval_data.pst[MIDGAME][color] += g_eval_params.PST_mg[color][piece][sq];
				g_eval_data.pst[ENDGAME][color] += g_eval_params.PST_eg[color][piece][sq];

				EvalScore piece_mobility = PieceMobility(piece, sq, pos.Occupancy(), pos.Pieces<color>());
				g_eval_data.mobility[MIDGAME][color] += piece_mobility.phase_score[MIDGAME];
				g_eval_data.mobility[ENDGAME][color] += piece_mobility.phase_score[ENDGAME];


				pieces &= pieces - 1;
			}

			return phase_change;
		}
	}

	// Evaluates the position from side to play's perspective.
	// Returns a score in centipawns.
    int GameState::ClassicalEvaluation() const
    {
		using namespace piece_type;
		g_eval_data = {};

		int phase = MAX_PHASE;
		phase -= EvaluatePieces<WHITE>(*this);
		phase -= EvaluatePieces<BLACK>(*this);

		// Insufficient Material Draws: KvK, KNvK, KBvK
		if (Pawns() == C64(0)) {
			int piece_material = g_eval_data.material[ENDGAME][WHITE] + g_eval_data.material[ENDGAME][BLACK];
			if (piece_material <= g_eval_params.piece_values[ENDGAME][BISHOP]) {
				return 0;
			}
		}	

		// bishop pair bonus
		int num_white_bishops = bitboard::PopCount(Pieces<WHITE, piece_type::BISHOP>());
		int num_black_bishops = bitboard::PopCount(Pieces<BLACK, piece_type::BISHOP>());

		if (num_white_bishops > 1) {
			g_eval_data.bishop_pair[MIDGAME][WHITE] += g_eval_params.bishop_pair_bonus[MIDGAME];
			g_eval_data.bishop_pair[ENDGAME][WHITE] += g_eval_params.bishop_pair_bonus[ENDGAME];
		}

		if (num_black_bishops > 1) {
			g_eval_data.bishop_pair[MIDGAME][BLACK] += g_eval_params.bishop_pair_bonus[MIDGAME];
			g_eval_data.bishop_pair[ENDGAME][BLACK] += g_eval_params.bishop_pair_bonus[ENDGAME];
		}



		EvaluatePawns<WHITE>(*this);
		EvaluatePawns<BLACK>(*this);
		
		g_eval_data.CalculateScore();

		// Map phase to [0-256]
		phase = ((phase << 8) + (MAX_PHASE >> 1)) / MAX_PHASE; 	// (phase * 256 + (MAX_PHASE / 2)) / MAX_PHASE
		g_eval_data.phase = phase;

		// Interpolate the score between middle game and end game.
		// (phase = 0 at the beginning and phase = 256 at the endgame.)
		int result = ((g_eval_data.score[MIDGAME] * (256 - phase)) + (g_eval_data.score[ENDGAME] * phase)) >> 8;



		if (m_side == WHITE) {
			result += g_eval_params.tempo_bonus;
			g_eval_data.tempo[WHITE] = g_eval_params.tempo_bonus;

		}
		if (m_side == BLACK) {
			result -= g_eval_params.tempo_bonus;
			g_eval_data.tempo[BLACK] = g_eval_params.tempo_bonus;
			result = (-result);
		}


		return result;
    }


}