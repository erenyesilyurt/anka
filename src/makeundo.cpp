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
		m_state_history[m_ply].zobrist_key = m_zobrist_key;
		m_state_history[m_ply].move_made = move;

		// remove hashes from key
		UpdateKeyWithEnPassant();
		UpdateKeyWithCastle();

		m_ep_target = square::NOSQUARE;

		Square from = move::FromSquare(move);
		Square to = move::ToSquare(move);
		Rank to_sq_rank = square::GetRank(to);
		File from_sq_file = square::GetFile(from);
		File to_sq_file = square::GetFile(to);
		u64 from_bb = C64(1) << from;
		u64 to_bb = C64(1) << to;
		u64 fromto_bb = from_bb | to_bb;
		PieceType moving_piece = move::MovingPiece(move);

		m_piecesBB[m_side] ^= fromto_bb;
		m_board[from] = piece_type::NOPIECE;

		// remove moving_piece hash from key
		UpdateKeyWithPiece(moving_piece, m_side, from);

		if (move::IsPromotion(move)) {
			PieceType promoted_piece = move::PromotedPiece(move);
			ANKA_ASSERT(moving_piece == piece_type::PAWN);
			// clear 'from' piece
			m_piecesBB[moving_piece] ^= from_bb;
			// add promoted piece
			m_piecesBB[promoted_piece] ^= to_bb;
			m_board[to] = promoted_piece;
			// add promoted_piece hash
			UpdateKeyWithPiece(promoted_piece, m_side, to);
		}
		else {
			m_piecesBB[moving_piece] ^= fromto_bb;
			m_board[to] = moving_piece;
			// add to_piece hash
			UpdateKeyWithPiece(moving_piece, m_side, to);
		}

		if (move::IsCapture(move)) {
			m_halfmove_clock = 0;

			if (move::IsEpCapture(move)) {
				Square cap_piece_sq = to - push_dir;
				Bitboard cap_piece_bb = C64(1) << cap_piece_sq;
				m_piecesBB[opposite_side] ^= cap_piece_bb;
				m_piecesBB[piece_type::PAWN] ^= cap_piece_bb;
				m_board[cap_piece_sq] = piece_type::NOPIECE;
				// remove captured pawn hash
				UpdateKeyWithPiece(piece_type::PAWN, opposite_side, cap_piece_sq);
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
			Square rook_from = square::H1;
			Square rook_to = square::F1;
			if (to_sq_file == file::C) {
				// queen side castle
				rook_from = square::A1;
				rook_to = square::D1;
			}

			rook_from ^=  flip_mask; // flip to rank 8 if black
			rook_to ^= flip_mask; // flip to rank 8 if black

			Bitboard rook_fromto_bb = (C64(1) << rook_from) | (C64(1) << rook_to);

			m_piecesBB[m_side] ^= rook_fromto_bb;
			m_piecesBB[piece_type::ROOK] ^= rook_fromto_bb;

			m_board[rook_from] = piece_type::NOPIECE;
			m_board[rook_to] = piece_type::ROOK;

			UpdateKeyWithPiece(piece_type::ROOK, m_side, rook_from);
			UpdateKeyWithPiece(piece_type::ROOK, m_side, rook_to);

		}
		else if (move::IsDoublePawnPush(move)) {
			m_ep_target = to - push_dir;
		}

		// update castle permissions
		m_castling_rights &= CastleRightsLUT[from];
		m_castling_rights &= CastleRightsLUT[to];

		m_halfmove_clock++;
		m_side = opposite_side;
		m_ply++;

		m_occupation = m_piecesBB[WHITE] | m_piecesBB[BLACK];
		// add hashes
		UpdateKeyWithCastle();
		UpdateKeyWithSide();
		UpdateKeyWithEnPassant();

		//PrintBitboards();
		ANKA_ASSERT(Validate());
	}


	void GameState::UndoMove()
	{
		// remove hashes
		UpdateKeyWithEnPassant();
		UpdateKeyWithCastle();

		m_ply = m_ply - 1;

		m_halfmove_clock = m_state_history[m_ply].half_move_clock;
		m_castling_rights = m_state_history[m_ply].castle_rights;
		m_ep_target = m_state_history[m_ply].ep_target;
		m_side = m_side ^ 1;

		// add hashes back
		UpdateKeyWithEnPassant();
		UpdateKeyWithCastle();
		UpdateKeyWithSide();

		int flip_mask = 56 & (~(m_side - 1)); // black: 56 white: 0
		int push_dir = 8 - (m_side << 4); // if black: south, if white: north
		Side opposite_side = static_cast<Side>(m_side ^ 1);

		Move move = m_state_history[m_ply].move_made;

		Square from = move::FromSquare(move);
		Square to = move::ToSquare(move);
		File to_sq_file = square::GetFile(to);
		u64 from_bb = C64(1) << from;
		u64 to_bb = C64(1) << to;
		u64 fromto_bb = from_bb | to_bb;
		PieceType moving_piece = move::MovingPiece(move);


		m_piecesBB[m_side] ^= fromto_bb;
		m_board[from] = moving_piece;
		m_board[to] = piece_type::NOPIECE;

		// add from_piece hash to the key
		UpdateKeyWithPiece(moving_piece, m_side, from);

		if (move::IsPromotion(move)) {
			PieceType promoted_piece = move::PromotedPiece(move);

			m_piecesBB[moving_piece] ^= from_bb;
			m_piecesBB[promoted_piece] ^= to_bb;

			// remove promoted_piece hash on to square
			UpdateKeyWithPiece(promoted_piece, m_side, to);
		}
		else {
			m_piecesBB[moving_piece] ^= fromto_bb;

			// remove moving piece hash on to square
			UpdateKeyWithPiece(moving_piece, m_side, to);
		}


		if (move::IsCapture(move)) {
			if (move::IsEpCapture(move)) {
				Square cap_piece_sq = to - push_dir;
				Bitboard cap_piece_bb = C64(1) << cap_piece_sq;
				m_piecesBB[opposite_side] ^= cap_piece_bb;
				m_piecesBB[piece_type::PAWN] ^= cap_piece_bb;
				m_board[cap_piece_sq] = piece_type::PAWN;
				// restore captured pawn hash
				UpdateKeyWithPiece(piece_type::PAWN, opposite_side, cap_piece_sq);
			}
			else {
				PieceType captured_piece = move::CapturedPiece(move);
				m_piecesBB[opposite_side] ^= to_bb;
				m_piecesBB[captured_piece] ^= to_bb;
				m_board[to] = captured_piece;
				// remove captured piece hash
				UpdateKeyWithPiece(captured_piece, opposite_side, to);
			}
		}
		else if (move::IsCastle(move)) {
			Square rook_from = square::H1;
			Square rook_to = square::F1;
			if (to_sq_file == file::C) {
				// queen side castle
				rook_from = square::A1;
				rook_to = square::D1;
			}

			rook_from ^= flip_mask; // flip to rank 8 if black
			rook_to ^= flip_mask; // flip to rank 8 if black
			Bitboard rook_fromto_bb = (C64(1) << rook_from) | (C64(1) << rook_to);

			m_piecesBB[m_side] ^= rook_fromto_bb;
			m_piecesBB[piece_type::ROOK] ^= rook_fromto_bb;

			m_board[rook_from] = piece_type::ROOK;
			m_board[rook_to] = piece_type::NOPIECE;

			UpdateKeyWithPiece(piece_type::ROOK, m_side, rook_from);
			UpdateKeyWithPiece(piece_type::ROOK, m_side, rook_to);

		}

		m_occupation = m_piecesBB[WHITE] | m_piecesBB[BLACK];
		ANKA_ASSERT(Validate());
	}

	void GameState::MakeNullMove()
	{
		m_state_history[m_ply].zobrist_key = m_zobrist_key;
		m_state_history[m_ply].ep_target = m_ep_target;
		m_state_history[m_ply].half_move_clock = m_halfmove_clock;
		m_state_history[m_ply].move_made = move::NULL_MOVE;

		UpdateKeyWithEnPassant();
		m_ep_target = square::NOSQUARE;
		m_halfmove_clock = 0; // TODO: increment the old value and change the repetition detection code?
		m_side = m_side ^ 1;
		UpdateKeyWithEnPassant();

		UpdateKeyWithSide();
		m_ply++;
	}

	void GameState::UndoNullMove()
	{
		UpdateKeyWithEnPassant();
		
		m_ply = m_ply - 1;
		m_side = m_side ^ 1;
		
		m_ep_target = m_state_history[m_ply].ep_target;
		m_halfmove_clock = m_state_history[m_ply].half_move_clock;
		UpdateKeyWithEnPassant();

		UpdateKeyWithSide();
	}
}