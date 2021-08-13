#pragma once

#include "bitboard.hpp"
#include "boarddefs.hpp"
#include "hash.hpp"
#include "attacks.hpp"
#include "move.hpp"
#include "util.hpp"
#include <string>


namespace anka {
	constexpr int kStateHistoryMaxSize = 1024;

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
			m_zobrist_key{}, m_ply{}, m_state_history{}, m_materials{0}
		{
			m_state_history = new PositionRecord[kStateHistoryMaxSize];
		}

		~GameState()
		{
			delete[] m_state_history;
		}

		force_inline Bitboard Occupancy() const { return m_occupation; }

		template <int side, int piece_type = piece_type::ALL>
		force_inline Bitboard Pieces() const
		{
			static_assert(side >= side::WHITE && side <= side::ALL);
			static_assert(piece_type >= piece_type::PAWN && piece_type <= piece_type::ALL);

			if constexpr (side == side::WHITE) {
				if constexpr (piece_type == piece_type::ALL)
					return m_piecesBB[side::WHITE];
				else
					return m_piecesBB[piece_type] & m_piecesBB[side::WHITE];
			}
			else if (side == side::BLACK) {
				if constexpr (piece_type == piece_type::ALL)
					return m_piecesBB[side::BLACK];
				else
					return m_piecesBB[piece_type] & m_piecesBB[side::BLACK];
			}
			else { // side::ALL
				if constexpr (piece_type == piece_type::ALL)
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
		force_inline Bitboard MajorPieces() const 
		{
			return m_piecesBB[piece_type::ROOK] | m_piecesBB[piece_type::QUEEN];
		}
		force_inline Bitboard MinorPieces() const
		{
			return m_piecesBB[piece_type::KNIGHT] | m_piecesBB[piece_type::BISHOP];
		}

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
		inline bool IsDrawn() const {
			// 50-move rule
			if (m_halfmove_clock > 99) {
				return true;
			}

			// insufficient material
			
			// KvK, KNvK, KBvK, TODO: KBvKB (same color bishops)
			if (TotalMaterial() <= MATERIAL_VALUES[piece_type::BISHOP]) {
				if (m_piecesBB[piece_type::PAWN] == 0) {
					return true;
				}
			}
			
			int ply_limit = Max(m_ply - m_halfmove_clock, 0);
			// repetition
			for (int ply = m_ply - 2; ply >= ply_limit; ply-=2) {
				if (m_zobrist_key == m_state_history[ply].zobrist_key) {
					return true;
				}
			}

			return false;
		}

		force_inline Square EnPassantSquare() const { return m_ep_target; }
		force_inline Side SideToPlay() const { return m_side; }
		force_inline int HalfMoveClock() const { return m_halfmove_clock; }
		force_inline byte CastlingRights() const { return m_castling_rights; }
		force_inline u64 PositionKey() const { return m_zobrist_key; }
		force_inline int Ply() const { return m_ply; }
		force_inline void SetPly(int ply) { m_ply = ply; }
		force_inline int Material(Side color) const { ANKA_ASSERT(color <= side::BLACK); return m_materials[color]; }
		force_inline int TotalMaterial() const { return m_materials[side::WHITE] + m_materials[side::BLACK]; }

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
		void MakeNullMove();
		void UndoNullMove();

		bool Validate();
		void Clear();
		bool LoadStartPosition() { return LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }
		bool LoadPosition(std::string fen);

		Move ParseMove(const char* line) const;
		void Print() const;
		void PrintBitboards() const;
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
		int m_ply; // 0 at the root of the state history (not always the root of the search tree)
		int m_materials[2];
		PositionRecord *m_state_history;
	};
} // namespace anka