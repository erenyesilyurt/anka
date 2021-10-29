#include "gamestate.hpp"
#include "move.hpp"

#include <iostream>

namespace anka {
	bool GameState::Validate()
	{
		bool valid = true;

		// validate castling rights
		if (m_castling_rights < 0 || m_castling_rights > 15) {
			std::cerr << "AnkaError (Validate): Castling rights corrupted in gamestate.\n";
			valid = false;
		}

		// validate zobrist key
		if (m_zobrist_key != CalculateKey()) {
			std::cerr << "AnkaError (Validate): Calculated key mismatch with incrementally updated key.\n";
			valid = false;
		}

		// validate mailbox and bitboards
		if ((Pieces<WHITE>() & Pieces<BLACK>()) != C64(0)) {
			std::cerr << "AnkaError (Validate): White Pieces and Black Pieces share elements.\n";
			valid = false;
		}

		for (Square sq = 0; sq < 64; sq++) {
			auto piece_type = m_board[sq];

			switch (piece_type)
			{
			case NO_PIECE:
				if (bitboard::BitIsSet(Occupancy(), sq)) {
					std::cerr << "AnkaError (Validate): Occupation BB/mailbox mismatch at square " << SquareToString(sq) <<"\n";
					valid = false;
				}
				break;
			case PAWN:
				if (!bitboard::BitIsSet(Pieces<ALLSIDES, PAWN>(), sq)) {
					std::cerr << "AnkaError (Validate): Pawns BB/mailbox mismatch at square " << SquareToString(sq) << "\n";
					valid = false;
				}
				break;
			case KNIGHT:
				if (!bitboard::BitIsSet(Pieces<ALLSIDES, KNIGHT>(), sq)) {
					std::cerr << "AnkaError (Validate): Knights BB/mailbox mismatch at square " << SquareToString(sq) << "\n";
					valid = false;
				}
				break;
			case BISHOP:
				if (!bitboard::BitIsSet(Pieces<ALLSIDES, BISHOP>(), sq)) {
					std::cerr << "AnkaError (Validate): Bishops BB/mailbox mismatch at square " << SquareToString(sq) << "\n";
					valid = false;
				}
				break;
			case ROOK:
				if (!bitboard::BitIsSet(Pieces<ALLSIDES, ROOK>(), sq)) {
					std::cerr << "AnkaError (Validate): Rooks BB/mailbox mismatch at square " << SquareToString(sq) << "\n";
					valid = false;
				}
				break;
			case QUEEN:
				if (!bitboard::BitIsSet(Pieces<ALLSIDES, QUEEN>(), sq)) {
					std::cerr << "AnkaError (Validate): Queens BB/mailbox mismatch at square " << SquareToString(sq) << "\n";
					valid = false;
				}
				break;
			case KING:
				if (!bitboard::BitIsSet(Pieces<ALLSIDES, KING>(), sq)) {
					std::cerr << "AnkaError (Validate): Kings BB/mailbox mismatch at square " << sq << "\n";
					valid = false;
				}
				break;
			default:
				std::cerr << "AnkaError (Validate): Invalid piece type (" << piece_type << ") at square " << sq << " in mailbox\n";
				valid = false;
				break;
			}
		}


		if (!valid)
			PrintBitboards();
		return valid;
	}

	bool move::Validate(Move m, Square from, Square to, PieceType moving_piece,
		bool is_prom, bool is_capture, bool is_castle,
		bool is_dpush, bool is_epcapture,
		PieceType cap_piece, PieceType prom_piece)
	{
		bool valid = true;

		if (move::FromSquare(m) != from) {
			std::cerr << "AnkaError (ValidateMove): From square mismatch.\n";
			valid = false;
		}

		if (move::ToSquare(m) != to) {
			std::cerr << "AnkaError (ValidateMove): To square mismatch.\n";
			valid = false;
		}

		if (move::MovingPiece(m) != moving_piece) {
			std::cerr << "AnkaError (ValidateMove): Moving piece mismatch.\n";
			valid = false;
		}

		if (move::IsPromotion(m) != is_prom) {
			std::cerr << "AnkaError (ValidateMove): Promotion flag mismatch.\n";
			valid = false;
		}

		if (move::IsCapture(m) != is_capture) {
			std::cerr << "AnkaError (ValidateMove): Capture flag mismatch.\n";
			valid = false;
		}

		if (move::IsEpCapture(m) != is_epcapture) {
			std::cerr << "AnkaError (ValidateMove): En passant flag mismatch.\n";
			valid = false;
		}


		if (move::IsQuiet(m)) {
			if (is_prom || is_capture || is_epcapture) {
				std::cerr << "AnkaError (ValidateMove): Quiet move has capture or promotion flags set.\n";
				valid = false;
			}
		}


		if (move::IsCastle(m) != is_castle) {
			std::cerr << "AnkaError (ValidateMove): Castle flag mismatch.\n";
			valid = false;
		}

		if (move::IsDoublePawnPush(m) != is_dpush) {
			std::cerr << "AnkaError (ValidateMove): Double pawn push flag mismatch.\n";
			valid = false;
		}


		if (move::CapturedPiece(m) != cap_piece) {
			std::cerr << "AnkaError (ValidateMove): Captured piece mismatch.\n";
			valid = false;
		}

		if (move::PromotedPiece(m) != prom_piece) {
			std::cerr << "AnkaError (ValidateMove): Promoted piece mismatch.\n";
			valid = false;
		}

		if (!valid) {
			std::cerr << "AnkaError (ValidateMove): Move: " << m <<"\n";
		}
		return valid;
	}
}