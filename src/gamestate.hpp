#pragma once

#include "bitboard.hpp"
#include "boarddefs.hpp"
#include "hash.hpp"
#include "attacks.hpp"
#include <string>
#include "move.hpp"

namespace anka {
	constexpr int kGameHistoryMaxSize = 1024;

	struct PositionRecord {
		u64 zobrist_key;
		Move move_made;
		int half_move_clock;
		Square ep_target;
		byte castle_rights;
	};

	class GameState {
	public:
		GameState() : m_piecesBB{}, m_occupation{}, m_board{}, m_ep_target{ square::NOSQUARE },
			m_side{ side::WHITE }, m_halfmove_clock{ 0 }, m_castling_rights{ 0 },
			m_zobrist_key{}, m_depth{}, m_game_history{}, m_total_material{0}
		{
			m_game_history = new PositionRecord[kGameHistoryMaxSize];
		}

		~GameState()
		{
			delete[] m_game_history;
		}

		force_inline Bitboard Occupancy() const { return m_occupation; }

		template <int side, int piece_type = piece_type::LOWERBOUND>
		force_inline Bitboard Pieces() const
		{
			static_assert(side >= side::WHITE && side <= side::LOWERBOUND);
			static_assert(piece_type >= piece_type::PAWN && piece_type <= piece_type::LOWERBOUND);

			if constexpr (side == side::WHITE) {
				if constexpr (piece_type == piece_type::LOWERBOUND)
					return m_piecesBB[side::WHITE];
				else
					return m_piecesBB[piece_type] & m_piecesBB[side::WHITE];
			}
			else if (side == side::BLACK) {
				if constexpr (piece_type == piece_type::LOWERBOUND)
					return m_piecesBB[side::BLACK];
				else
					return m_piecesBB[piece_type] & m_piecesBB[side::BLACK];
			}
			else { // side::ALL
				if constexpr (piece_type == piece_type::LOWERBOUND)
					return m_piecesBB[side::WHITE] & m_piecesBB[side::BLACK];
				else
					return m_piecesBB[piece_type];
			}
		}

		force_inline Bitboard WhitePieces() const { return m_piecesBB[side::WHITE]; }
		force_inline Bitboard BlackPieces() const { return m_piecesBB[side::BLACK]; }
		force_inline Bitboard Pawns() const { return m_piecesBB[piece_type::PAWN]; }
		force_inline Bitboard Knights() const { return m_piecesBB[piece_type::KNIGHT]; }
		force_inline Bitboard Bishops() const { return m_piecesBB[piece_type::BISHOP]; }
		force_inline Bitboard Rooks() const { return m_piecesBB[piece_type::ROOK]; }
		force_inline Bitboard Queens() const { return m_piecesBB[piece_type::QUEEN]; }
		force_inline Bitboard Kings() const { return m_piecesBB[piece_type::KING]; }

		force_inline PieceType GetPiece(Square square) const { return m_board[square]; }

		template <int attacking_side>
		inline bool IsAttacked(Square square) const
		{
			using namespace piece_type;
			constexpr int defending_side = attacking_side ^ 1;

			Bitboard att_knights = Pieces<attacking_side, KNIGHT>();
			if (attacks::KnightAttacks(square) & att_knights)
				return true;

			Bitboard att_queens = Pieces<attacking_side, QUEEN>();
			Bitboard att_bishops_and_queens = Pieces<attacking_side, BISHOP>() | att_queens;
			if (attacks::BishopAttacks(square, m_occupation) & att_bishops_and_queens)
				return true;

			Bitboard att_rooks_and_queens = Pieces<attacking_side, ROOK>() | att_queens;
			if (attacks::RookAttacks(square, m_occupation) & att_rooks_and_queens)
				return true;

			Bitboard att_pawns = Pieces<attacking_side, PAWN>();
			if (attacks::PawnAttacks<defending_side>(square) & att_pawns)
				return true;

			Bitboard att_kings = Pieces<attacking_side, KING>();
			if (attacks::KingAttacks(square) & att_kings)
				return true;

			return false;
		}

		inline bool InCheck() const
		{
			Bitboard king_bb = m_piecesBB[m_side] & m_piecesBB[piece_type::KING];
			Square king_sq = bitboard::BitScanForward(king_bb);
			ANKA_ASSERT(king_sq >= square::A1 && king_sq <= square::H8);
			int opposite_side = m_side ^ 1;

			Bitboard att_knights = m_piecesBB[opposite_side] & m_piecesBB[piece_type::KNIGHT];
			if (attacks::KnightAttacks(king_sq) & att_knights)
				return true;

			Bitboard att_bishops_and_queens = m_piecesBB[opposite_side] & (m_piecesBB[piece_type::BISHOP] | m_piecesBB[piece_type::QUEEN]);
			if (attacks::BishopAttacks(king_sq, m_occupation) & att_bishops_and_queens)
				return true;

			Bitboard att_rooks_and_queens = m_piecesBB[opposite_side] & (m_piecesBB[piece_type::ROOK] | m_piecesBB[piece_type::QUEEN]);
			if (attacks::RookAttacks(king_sq, m_occupation) & att_rooks_and_queens)
				return true;

			Bitboard att_pawns = m_piecesBB[opposite_side] & m_piecesBB[piece_type::PAWN];
			if (attacks::PawnAttacks(king_sq, m_side) & att_pawns)
				return true;

			return false;
		}

		// TODO: test this
		force_inline bool IsRepetition() const
		{
			int history_index = m_depth - 1;
			int i = 0;
			while (history_index >= 0) {
				if (i == m_halfmove_clock)
					break;
				if (m_zobrist_key == m_game_history[history_index].zobrist_key)
					return true;
				i++;
				history_index--;
			}

			return false;
		}

		force_inline Square EnPassantSquare() const { return m_ep_target; }
		force_inline Side SideToPlay() const { return m_side; }
		force_inline int HalfMoveClock() const { return m_halfmove_clock; }
		force_inline byte CastlingRights() const { return m_castling_rights; }
		force_inline u64 PositionKey() const { return m_zobrist_key; }
		force_inline int SearchDepth() const { return m_depth; }
		force_inline void SetSearchDepth(int d) { m_depth = d; }
		force_inline int TotalMaterial() const { return m_total_material; }

		u64 CalculateKey();

		force_inline void UpdateKeyWithPiece(PieceType piece_type, Side piece_color, Square sq)
		{
			ANKA_ASSERT(piece_color == side::WHITE || piece_color == side::BLACK);
			ANKA_ASSERT(sq >=0 && sq < 64);
			ANKA_ASSERT(piece_type >= piece_type::PAWN  && piece_type <= piece_type::KING);
			m_zobrist_key ^= zobrist_keys::piece_keys[piece_color][piece_type - 2][sq];
		}

		force_inline void UpdateKeyWithCastle()
		{
			m_zobrist_key ^= zobrist_keys::castle_keys[m_castling_rights];
		}

		force_inline void UpdateKeyWithSide()
		{
			m_zobrist_key ^= zobrist_keys::side_key;
		}

		force_inline void UpdateKeyWithEnPassant()
		{
			m_zobrist_key ^= zobrist_keys::ep_keys[m_ep_target];
		}

		void MakeMove(Move move);
		void UndoMove();

		bool Validate();
		void Clear();
		bool LoadStartPosition() { return LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }
		bool LoadPosition(std::string fen);

		void Print();
		void PrintBitboards();
	private:
		/* BITBOARDS
		0: White pieces
		1: Black pieces
		2: Pawns
		3: Knights
		4: Bishops
		5: Rooks
		6: Queens
		7: Kings
		*/
		Bitboard m_piecesBB[8];
		Bitboard m_occupation;

		// Mailbox
		PieceType m_board[64];

		Square m_ep_target;
		Side m_side;
		int m_halfmove_clock;
		byte m_castling_rights;
		u64 m_zobrist_key;
		int m_depth;
		int m_total_material;
		PositionRecord *m_game_history;
	};
} // namespace anka