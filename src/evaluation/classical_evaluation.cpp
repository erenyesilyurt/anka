#include "evaluation.hpp"
#include "boarddefs.hpp"
#include "movegen.hpp"


namespace anka {

	EvalParams eval_params;
	EvalInfo eval_data;

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
			score.phase_score[MIDGAME] = eval_params.mobility_weights[MIDGAME][piece] * num_attacks;
			score.phase_score[ENDGAME] = eval_params.mobility_weights[ENDGAME][piece] * num_attacks;
			return score;
		}

		template<Side color>
		void EvaluatePawns(EvalInfo& eval_data, const GameState& pos)
		{
			using namespace piece_type;
			constexpr Side opposite_color = color ^ 1;
			Bitboard ally_pawns = pos.Pieces<color, PAWN>();
			Bitboard opponent_pawns = pos.Pieces<opposite_color, PAWN>();

			Bitboard pawns = ally_pawns;
			while (pawns) {
				Square sq = bitboard::BitScanForward(pawns);
				eval_data.material[MIDGAME][color] += eval_params.piece_values[MIDGAME][PAWN];
				eval_data.material[ENDGAME][color] += eval_params.piece_values[ENDGAME][PAWN];
				eval_data.pst[MIDGAME][color] += eval_params.PST_mg[color][PAWN][sq];
				eval_data.pst[ENDGAME][color] += eval_params.PST_eg[color][PAWN][sq];


				//// check if passed pawn
				if ((attacks::PawnFrontSpan(sq, color) & opponent_pawns) == 0) {
					int rank = square::GetRank(sq);
					if constexpr (color == BLACK)
						rank = 7 - rank;
					eval_data.pawn_structure[MIDGAME][color] += eval_params.passed_bonus[MIDGAME][rank];
					eval_data.pawn_structure[ENDGAME][color] += eval_params.passed_bonus[ENDGAME][rank];

				}

				//// check if isolated pawn
				int file = square::GetFile(sq);
				if ((attacks::AdjacentFiles(file) & ally_pawns) == 0) {
					eval_data.pawn_structure[MIDGAME][color] -= eval_params.isolated_penalty[MIDGAME][file];
					eval_data.pawn_structure[ENDGAME][color] -= eval_params.isolated_penalty[ENDGAME][file];
				}

				pawns &= pawns - 1;
			}
		}


		template<Side color>
		int EvaluatePieces(EvalInfo& eval_data, const GameState& pos)
		{
			using namespace piece_type;

			int phase_change = 0;
			Bitboard pieces = pos.Pieces<color>() ^ pos.Pieces<color, PAWN>();
			while (pieces) {
				Square sq = bitboard::BitScanForward(pieces);
				auto piece = pos.GetPiece(sq);
				phase_change += p_phase[piece];

				eval_data.material[MIDGAME][color] += eval_params.piece_values[MIDGAME][piece];
				eval_data.material[ENDGAME][color] += eval_params.piece_values[ENDGAME][piece];

				eval_data.pst[MIDGAME][color] += eval_params.PST_mg[color][piece][sq];
				eval_data.pst[ENDGAME][color] += eval_params.PST_eg[color][piece][sq];

				EvalScore piece_mobility = PieceMobility(piece, sq, pos.Occupancy(), pos.Pieces<color>());
				eval_data.mobility[MIDGAME][color] += piece_mobility.phase_score[MIDGAME];
				eval_data.mobility[ENDGAME][color] += piece_mobility.phase_score[ENDGAME];


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
		eval_data = {};

		int phase = MAX_PHASE;
		phase -= EvaluatePieces<WHITE>(eval_data, *this);
		phase -= EvaluatePieces<BLACK>(eval_data, *this);

		// Insufficient Material Draws: KvK, KNvK, KBvK
		if (Pawns() == C64(0)) {
			int piece_material = eval_data.material[ENDGAME][WHITE] + eval_data.material[ENDGAME][BLACK];
			if (piece_material <= eval_params.piece_values[ENDGAME][BISHOP]) {
				return 0;
			}
		}	

		// bishop pair bonus
		int num_white_bishops = bitboard::PopCount(Pieces<WHITE, piece_type::BISHOP>());
		int num_black_bishops = bitboard::PopCount(Pieces<BLACK, piece_type::BISHOP>());

		if (num_white_bishops > 1) {
			eval_data.bishop_pair[MIDGAME][WHITE] += eval_params.bishop_pair_bonus[MIDGAME];
			eval_data.bishop_pair[ENDGAME][WHITE] += eval_params.bishop_pair_bonus[ENDGAME];
		}

		if (num_black_bishops > 1) {
			eval_data.bishop_pair[MIDGAME][BLACK] += eval_params.bishop_pair_bonus[MIDGAME];
			eval_data.bishop_pair[ENDGAME][BLACK] += eval_params.bishop_pair_bonus[ENDGAME];
		}



		EvaluatePawns<WHITE>(eval_data, *this);
		EvaluatePawns<BLACK>(eval_data, *this);
		
		eval_data.CalculateScore();

		// Map phase to [0-256]
		phase = ((phase << 8) + (MAX_PHASE >> 1)) / MAX_PHASE; 	// (phase * 256 + (MAX_PHASE / 2)) / MAX_PHASE
		eval_data.phase = phase;

		// Interpolate the score between middle game and end game.
		// (phase = 0 at the beginning and phase = 256 at the endgame.)
		int result = ((eval_data.score[MIDGAME] * (256 - phase)) + (eval_data.score[ENDGAME] * phase)) >> 8;



		if (m_side == WHITE) {
			result += eval_params.tempo_bonus;
			eval_data.tempo[WHITE] = eval_params.tempo_bonus;

		}
		if (m_side == BLACK) {
			result -= eval_params.tempo_bonus;
			eval_data.tempo[BLACK] = eval_params.tempo_bonus;
			result = (-result);
		}


		return result;
    }


}