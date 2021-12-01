#include "gamestate.hpp"
#include "move.hpp"
#include <assert.h>


static byte CastleRightsLUT[64]{
	0x0b, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0x07,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x0e, 0xff, 0xff, 0xff, 0x0c, 0xff, 0xff, 0x0d
};


namespace anka {

	void GameState::MakeMove(Move move)
	{
		Side opposite_side = m_side ^ 1;
		int flip_mask = 56 & (~(m_side - 1)); // if black: 56 , if white: 0
		int push_dir = 8 - (m_side << 4); // if black: south, if white: north

		m_state_history[m_ply].castle_rights = m_castling_rights;
		m_state_history[m_ply].ep_target = m_ep_target;
		m_state_history[m_ply].half_move_clock = m_halfmove_clock;
		m_state_history[m_ply].move_made = move;
		m_key_history[m_root_ply_index + m_ply] = m_zobrist_key;

		// remove hashes from key
		UpdateKeyWithEnPassant();
		UpdateKeyWithCastle();

		m_halfmove_clock++;
		m_ep_target = NO_SQUARE;

		Square from = move::FromSquare(move);
		Square to = move::ToSquare(move);
		File to_sq_file = GetFile(to);
		u64 from_bb = C64(1) << from;
		u64 to_bb = C64(1) << to;
		u64 fromto_bb = from_bb | to_bb;
		PieceType moving_piece = move::MovingPiece(move);

		m_piecesBB[m_side] ^= fromto_bb;
		m_board[from] = NO_PIECE;

		// remove moving_piece hash from key
		UpdateKeyWithPiece(moving_piece, m_side, from);
		m_piecesBB[moving_piece] ^= fromto_bb;
		m_board[to] = moving_piece;
		// add moving_piece 'to' hash
		UpdateKeyWithPiece(moving_piece, m_side, to);


		if (moving_piece == PAWN) {
			m_halfmove_clock = 0;
			if (move::IsDoublePawnPush(move)) {
				m_ep_target = to - push_dir;
			}
			else if (move::IsPromotion(move)) {
				// undo moving_piece target sq hash
				UpdateKeyWithPiece(moving_piece, m_side, to);
				// undo moving_piece target sq bit
				m_piecesBB[moving_piece] ^= to_bb;

				PieceType promoted_piece = move::PromotedPiece(move);
				// add promoted piece
				m_piecesBB[promoted_piece] ^= to_bb;
				m_board[to] = promoted_piece;
				// add promoted_piece hash
				UpdateKeyWithPiece(promoted_piece, m_side, to);
			}
		}


		if (move::IsCapture(move)) {
			m_halfmove_clock = 0;

			if (move::IsEpCapture(move)) {
				Square cap_piece_sq = to - push_dir;
				Bitboard cap_piece_bb = C64(1) << cap_piece_sq;
				m_piecesBB[opposite_side] ^= cap_piece_bb;
				m_piecesBB[PAWN] ^= cap_piece_bb;
				m_board[cap_piece_sq] = NO_PIECE;
				// remove captured pawn hash
				UpdateKeyWithPiece(PAWN, opposite_side, cap_piece_sq);
			}
			else {
				PieceType captured_piece = move::CapturedPiece(move);
				m_piecesBB[opposite_side] ^= to_bb;
				m_piecesBB[captured_piece] ^= to_bb;
				// remove captured piece hash
				UpdateKeyWithPiece(captured_piece, opposite_side, to);
			}
		}
		else if (move::IsCastle(move)) {
			Square rook_from = H1;
			Square rook_to = F1;
			if (to_sq_file == FILE_C) {
				// queen side castle
				rook_from = A1;
				rook_to = D1;
			}

			rook_from ^=  flip_mask; // flip to rank 8 if black
			rook_to ^= flip_mask; // flip to rank 8 if black

			Bitboard rook_fromto_bb = (C64(1) << rook_from) | (C64(1) << rook_to);

			m_piecesBB[m_side] ^= rook_fromto_bb;
			m_piecesBB[ROOK] ^= rook_fromto_bb;

			m_board[rook_from] = NO_PIECE;
			m_board[rook_to] = ROOK;

			UpdateKeyWithPiece(ROOK, m_side, rook_from);
			UpdateKeyWithPiece(ROOK, m_side, rook_to);
		}

		// update castle permissions
		m_castling_rights &= CastleRightsLUT[from];
		m_castling_rights &= CastleRightsLUT[to];

		
		m_side = opposite_side;
		m_ply++;

		m_occupation = m_piecesBB[WHITE] | m_piecesBB[BLACK];
		// add hashes
		UpdateKeyWithCastle();
		UpdateKeyWithSide();
		UpdateKeyWithEnPassant();

		ANKA_ASSERT(Validate());
	}


	void GameState::UndoMove()
	{
		m_ply = m_ply - 1;
		m_side = m_side ^ 1;
		int flip_mask = 56 & (~(m_side - 1)); // black: 56 white: 0
		int push_dir = 8 - (m_side << 4); // if black: south, if white: north
		Side opposite_side = static_cast<Side>(m_side ^ 1);

		m_halfmove_clock = m_state_history[m_ply].half_move_clock;
		m_castling_rights = m_state_history[m_ply].castle_rights;
		m_ep_target = m_state_history[m_ply].ep_target;
		Move move = m_state_history[m_ply].move_made;
		Square from = move::FromSquare(move);
		Square to = move::ToSquare(move);
		File to_sq_file = GetFile(to);
		u64 from_bb = C64(1) << from;
		u64 to_bb = C64(1) << to;
		u64 fromto_bb = from_bb | to_bb;
		PieceType moving_piece = move::MovingPiece(move);


		m_piecesBB[m_side] ^= fromto_bb;
		m_board[from] = moving_piece;
		m_board[to] = NO_PIECE;

		if (move::IsPromotion(move)) {
			PieceType promoted_piece = move::PromotedPiece(move);

			m_piecesBB[moving_piece] ^= from_bb;
			m_piecesBB[promoted_piece] ^= to_bb;
		}
		else {
			m_piecesBB[moving_piece] ^= fromto_bb;
		}


		if (move::IsCapture(move)) {
			if (move::IsEpCapture(move)) {
				Square cap_piece_sq = to - push_dir;
				Bitboard cap_piece_bb = C64(1) << cap_piece_sq;
				m_piecesBB[opposite_side] ^= cap_piece_bb;
				m_piecesBB[PAWN] ^= cap_piece_bb;
				m_board[cap_piece_sq] = PAWN;
			}
			else {
				PieceType captured_piece = move::CapturedPiece(move);
				m_piecesBB[opposite_side] ^= to_bb;
				m_piecesBB[captured_piece] ^= to_bb;
				m_board[to] = captured_piece;
			}
		}
		else if (move::IsCastle(move)) {
			Square rook_from = H1;
			Square rook_to = F1;
			if (to_sq_file == FILE_C) {
				// queen side castle
				rook_from = A1;
				rook_to = D1;
			}

			rook_from ^= flip_mask; // flip to rank 8 if black
			rook_to ^= flip_mask; // flip to rank 8 if black
			Bitboard rook_fromto_bb = (C64(1) << rook_from) | (C64(1) << rook_to);

			m_piecesBB[m_side] ^= rook_fromto_bb;
			m_piecesBB[ROOK] ^= rook_fromto_bb;

			m_board[rook_from] = ROOK;
			m_board[rook_to] = NO_PIECE;
		}

		m_zobrist_key = m_key_history[m_root_ply_index + m_ply];
		m_occupation = m_piecesBB[WHITE] | m_piecesBB[BLACK];
		ANKA_ASSERT(Validate());
	}

	void GameState::MakeNullMove()
	{
		m_key_history[m_root_ply_index + m_ply] = m_zobrist_key;
		m_state_history[m_ply].ep_target = m_ep_target;
		m_state_history[m_ply].half_move_clock = m_halfmove_clock;
		m_state_history[m_ply].move_made = move::NULL_MOVE;

		UpdateKeyWithEnPassant();
		m_ep_target = NO_SQUARE;
		m_halfmove_clock = 0; // treat null move as an irreversible move
		m_side = m_side ^ 1;
		UpdateKeyWithEnPassant();
		UpdateKeyWithSide();

		m_ply++;
	}

	void GameState::UndoNullMove()
	{	
		m_ply = m_ply - 1;
		m_side = m_side ^ 1;	
		m_ep_target = m_state_history[m_ply].ep_target;
		m_halfmove_clock = m_state_history[m_ply].half_move_clock;
		m_zobrist_key = m_key_history[m_root_ply_index + m_ply];
	}
}