#include "evaluation.hpp"
#include "boarddefs.hpp"

namespace anka {
	
		static constexpr int pawns_pst[]{
			 0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			 5,  5, 10, 25, 25, 10,  5,  5,
			 0,  0,  0, 20, 20,  0,  0,  0,
			 5, -5,-10,  0,  0,-10, -5,  5,
			 5, 10, 10,-20,-20, 10, 10,  5,
			 0,  0,  0,  0,  0,  0,  0,  0
		};

		static constexpr int knights_pst[]{
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50,
		};

		static constexpr int bishops_pst []{
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5, 10, 10,  5,  0,-10,
			-10,  5,  5, 10, 10,  5,  5,-10,
			-10,  0, 10, 10, 10, 10,  0,-10,
			-10, 10, 10, 10, 10, 10, 10,-10,
			-10,  5,  0,  0,  0,  0,  5,-10,
			-20,-10,-10,-10,-10,-10,-10,-20,
		};

		static constexpr int rooks_pst[]{
			  0,  0,  0,  0,  0,  0,  0,  0,
			  5, 10, 10, 10, 10, 10, 10,  5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			  0,  0,  0,  5,  5,  0,  0,  0
		};

		static constexpr int queens_pst[]{
			-20,-10,-10, -5, -5,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5,  5,  5,  5,  0,-10,
			 -5,  0,  5,  5,  5,  5,  0, -5,
			  0,  0,  5,  5,  5,  5,  0, -5,
			-10,  5,  5,  5,  5,  5,  0,-10,
			-10,  0,  5,  0,  0,  0,  0,-10,
			-20,-10,-10, -5, -5,-10,-10,-20
		};

		static constexpr int king_pst[]{
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-10,-20,-20,-20,-20,-20,-20,-10,
			 20, 20,  0,  0,  0,  0, 20, 20,
			 20, 30, 10,  0,  0, 10, 30, 20
		};

		static constexpr int king_endgame_pst[]{
			-50,-40,-30,-20,-20,-30,-40,-50,
			-30,-20,-10,  0,  0,-10,-20,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 30, 40, 40, 30,-10,-30,
			-30,-10, 20, 30, 30, 20,-10,-30,
			-30,-30,  0,  0,  0,  0,-30,-30,
			-50,-30,-30,-30,-30,-30,-30,-50
		};

    // an implementation of the simple evaluation function proposed by Tomasz Michniewski
	// Evaluates the position from white's perspective then negates the score if side to play is black
    int ClassicEvaluation(const GameState& pos)
    {
		Side side = pos.SideToPlay();
		int flip_mask = 56 & (side - 1); // if black: 0 , if white: 56

		int score = 0;

		int num_pawns = bitboard::PopCount(pos.Pawns());
		int num_w_pawns = bitboard::PopCount(pos.Pawns() & pos.WhitePieces());
		int num_b_pawns = num_pawns - num_w_pawns;

		int num_knights = bitboard::PopCount(pos.Knights());
		int num_w_knights = bitboard::PopCount(pos.Knights() & pos.WhitePieces());
		int num_b_knights = num_knights - num_w_knights;

		int num_bishops = bitboard::PopCount(pos.Bishops());
		int num_w_bishops = bitboard::PopCount(pos.Bishops() & pos.WhitePieces());
		int num_b_bishops = num_bishops - num_w_bishops;

		int num_rooks = bitboard::PopCount(pos.Rooks());
		int num_w_rooks = bitboard::PopCount(pos.Rooks() & pos.WhitePieces());
		int num_b_rooks = num_rooks - num_w_rooks;

		int num_queens = bitboard::PopCount(pos.Queens());
		int num_w_queens = bitboard::PopCount(pos.Queens() & pos.WhitePieces());
		int num_b_queens = num_queens - num_w_queens;

		score += (num_w_pawns - num_b_pawns) * MATERIAL_VALUES[piece_type::PAWN];
		score += (num_w_knights - num_b_knights) * MATERIAL_VALUES[piece_type::KNIGHT];
		score += (num_w_bishops - num_b_bishops) * MATERIAL_VALUES[piece_type::BISHOP];
		score += (num_w_rooks - num_b_rooks) * MATERIAL_VALUES[piece_type::ROOK];
		score += (num_w_queens - num_b_queens) * MATERIAL_VALUES[piece_type::QUEEN];
		// { 0, 0, 100, 300, 300, 500, 900, 0 };
		int total_material = pos.TotalMaterial();

		// calculate phase
		constexpr int max_material = 7800; // start of game with all pieces on the board
		constexpr int endgame_material = 1200; // 3 pawns and 1 bishop remaining per side

		float phase = 1.0f;
		if (total_material < endgame_material) {
			phase = 0.0f;
		}
		else { // interpolate the phase
			// phase: 1 -> beginning, phase: 0 -> endgame
			phase = ((float)(total_material - endgame_material)) / (max_material - endgame_material);
			ANKA_ASSERT(phase >= 0 || phase <= 1.0f);
		}

		u64 black_pieces = pos.BlackPieces();
		while (black_pieces) {
			Square sq = bitboard::BitScanForward(black_pieces);

			auto piece = pos.GetPiece(sq);
			int bonus_score = 0;

			switch (piece) {
			case piece_type::PAWN:
				bonus_score = pawns_pst[sq];
				break;
			case piece_type::KNIGHT:
				bonus_score = knights_pst[sq];
				break;
			case piece_type::BISHOP:
				bonus_score = bishops_pst[sq];
				break;
			case piece_type::ROOK:
				bonus_score = rooks_pst[sq];
				break;
			case piece_type::QUEEN:
				bonus_score = queens_pst[sq];
				break;
			case piece_type::KING:
				bonus_score = (phase * king_pst[sq]) + ((1.0f - phase) * king_endgame_pst[sq]);
				break;
			}

			score -= bonus_score;
			black_pieces &= black_pieces - 1;
		}

		// PST bonus score additions
		u64 white_pieces = pos.WhitePieces();
		while (white_pieces) {
			Square sq = bitboard::BitScanForward(white_pieces);

			auto piece = pos.GetPiece(sq);
			int bonus_score = 0;
			sq ^= 56; // vertically flip the square when side to play is black for pst lookup

			switch (piece) {
			case piece_type::PAWN:
				bonus_score = pawns_pst[sq];
				break;
			case piece_type::KNIGHT:
				bonus_score = knights_pst[sq];
				break;
			case piece_type::BISHOP:
				bonus_score = bishops_pst[sq];
				break;
			case piece_type::ROOK:
				bonus_score = rooks_pst[sq];
				break;
			case piece_type::QUEEN:
				bonus_score = queens_pst[sq];
				break;
			case piece_type::KING:
				bonus_score = (phase * king_pst[sq]) + ((1.0f - phase) * king_endgame_pst[sq]);
				break;
			}

			score += bonus_score;
			white_pieces &= white_pieces - 1;
		}




		int tempo_bonus = 5;
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