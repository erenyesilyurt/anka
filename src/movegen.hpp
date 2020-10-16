#pragma once
#include "attacks.hpp"
#include "gamestate.hpp"
#include "move.hpp"

/* MOVE ENCODING
Moves are encoded as 32-bit integers.
Starting from the LSB:
[0..5] : From square
[6..11]: To square
[12]   : promotion flag
[13]   : capture flag
[14]   : castle flag
[15]   : double pawn push flag
[16]   : ep capture flag
[17..19]: captured piece type
[20..22]: promoted piece type
[23..25]: moving piece type

*/

namespace anka {

	inline constexpr int kMoveListMaxSize = 256;

	template <size_t n>
	class MoveList {
	public:
		GradedMove moves[n];
		int length = 0;

		// generates legal moves. returns true if in check
		// if quiescence is true, only generates check evasions and captures
		bool GenerateLegalMoves(const GameState& pos, bool quiescence = false);

		void GenerateLegalCaptures(const GameState& pos);
		

		// removes the highest ranked move from the list and returns it
		// CHECK IF LIST IS EMPTY BEFORE CALLING
		Move PopBest()
		{
			int best_index = 0;
			int best_score = moves[0].score;

			for (int i = 1; i < length; i++) {
				if (moves[i].score > best_score) {
					best_score = moves[i].score;
					best_index = i;
				}
			}

			Move result = moves[best_index].move;
			// Pop the best move
			GradedMove tmp = moves[best_index];
			moves[best_index] = moves[length - 1];
			moves[length - 1] = tmp;
			length--;

			return result;
		}

		void PrintMoves() const;

		force_inline bool Find(Move m) const
		{
			for (int i = 0; i < length; i++) {
				if (m == moves[i].move)
					return true;
			}
			return false;
		}

		// set order score of move if it exists
		force_inline void SetMoveScore(Move m, int score)
		{
			for (int i = 0; i < length; i++) {
				if (m == moves[i].move) {
					moves[i].score = score;
					return;
				}
			}
		}

	private:
		void GenerateNonPawnMoves(const GameState& pos,
			Bitboard ally_pieces, Bitboard opp_pieces,
			Bitboard occ, Bitboard free_mask,
			Bitboard capturables, Bitboard pushables);


		template <int side, int piece>
		void GeneratePieceMoves(const GameState &pos, Bitboard ally_pieces,
			Bitboard opp_pieces, Bitboard free_to_move,
			Bitboard pushables, Bitboard capturables)
		{
			using namespace piece_type;
			static_assert(piece >= KNIGHT && piece <= QUEEN, "Piece type must be a major or minor piece");

			ally_pieces &= pos.Pieces<side, piece>() & free_to_move;

			Bitboard occ = pos.Occupancy();
			Bitboard empty = ~occ;
			while (ally_pieces) {
				Square from = bitboard::BitScanForward(ally_pieces);
				Bitboard attacks;
				if constexpr (piece == KNIGHT)
					attacks = attacks::KnightAttacks(from);
				else if (piece == BISHOP)
					attacks = attacks::BishopAttacks(from, occ);
				else if (piece == ROOK)
					attacks = attacks::RookAttacks(from, occ);
				else if (piece == QUEEN)
					attacks = attacks::BishopAttacks(from, occ) | attacks::RookAttacks(from, occ);
				Bitboard cap_targets = attacks & opp_pieces & capturables;
				Bitboard push_targets = attacks & empty & pushables;
				while (cap_targets) {
					Square to = bitboard::BitScanForward(cap_targets);
					AddCapture<piece>(from, to, pos.GetPiece(to));
					cap_targets &= cap_targets - 1;
				}
				while (push_targets) {
					Square to = bitboard::BitScanForward(push_targets);
					AddQuiet<piece>(from, to);
					push_targets &= push_targets - 1;
				}
				ally_pieces &= ally_pieces - 1;
			}
		}

		template <int side>
		void GenerateCastleMoves(const GameState &pos)
		{
			Bitboard occ = pos.Occupancy();
			Bitboard castle_k_empty_mask = (C64(1) << square::F1) | (C64(1) << square::G1);
			//Bitboard castle_k_noattack_mask = (C64(1) << square::E1) | (C64(1) << square::F1) | (C64(1) << square::G1);
			Bitboard castle_q_empty_mask = (C64(1) << square::B1) | (C64(1) << square::C1) | (C64(1) << square::D1);
			//Bitboard castle_q_noattack_mask = (C64(1) << square::C1) | (C64(1) << square::D1) | (C64(1) << square::E1);
			Bitboard castle_k = CastlePermFlags_t::castle_wk;
			Bitboard castle_q = CastlePermFlags_t::castle_wq;
			if constexpr (side == side::BLACK) {
				castle_k_empty_mask = (C64(1) << square::F8) | (C64(1) << square::G8);
				//castle_k_noattack_mask = (C64(1) << square::E8) | (C64(1) << square::F8) | (C64(1) << square::G8);
				castle_q_empty_mask = (C64(1) << square::B8) | (C64(1) << square::C8) | (C64(1) << square::D8);
				//castle_q_noattack_mask = (C64(1) << square::C8) | (C64(1) << square::D8) | (C64(1) << square::E8);
				castle_k = CastlePermFlags_t::castle_bk;
				castle_q = CastlePermFlags_t::castle_bq;
			}

			// king side castle
			if (pos.CastlingRights() & castle_k) {
				if ((castle_k_empty_mask & occ) == 0) {
					if constexpr (side == side::WHITE) {
						if (!pos.IsAttacked<side::BLACK>(square::E1)
							&& !pos.IsAttacked<side::BLACK>(square::F1)
							&& !pos.IsAttacked<side::BLACK>(square::G1))
						{
							AddCastle(square::E1, square::G1);
						}
					}
					else { // black
						if (!pos.IsAttacked<side::WHITE>(square::E8)
							&& !pos.IsAttacked<side::WHITE>(square::F8)
							&& !pos.IsAttacked<side::WHITE>(square::G8))
						{
							AddCastle(square::E8, square::G8);
						}
					}
				}
			}

			// queen side castle
			if (pos.CastlingRights() & castle_q) {
				if ((castle_q_empty_mask & occ) == 0) {
					if constexpr (side == side::WHITE) {
						if (!pos.IsAttacked<side::BLACK>(square::E1)
							&& !pos.IsAttacked<side::BLACK>(square::D1)
							&& !pos.IsAttacked<side::BLACK>(square::C1))
						{
							AddCastle(square::E1, square::C1);
						}
					}
					else { // black
						if (!pos.IsAttacked<side::WHITE>(square::E8)
							&& !pos.IsAttacked<side::WHITE>(square::D8)
							&& !pos.IsAttacked<side::WHITE>(square::C8))
						{
							AddCastle(square::E8, square::C8);
						}
					}
				}
			}
		}

		template <int side>
		void GeneratePawnPushMoves(const GameState& pos, Bitboard ally_pawns, Bitboard legal_squares)
		{
			constexpr int push_dir = direction::N - (side << 4); // south if black
			u64 dpush_rank_mask;
			u64 prom_rank_mask;
			if constexpr (side == side::WHITE) {
				dpush_rank_mask = C64(0xFF000000); // rank 4
				prom_rank_mask = C64(0xFF00000000000000); // rank 8
			}
			else {
				dpush_rank_mask = C64(0xFF00000000); // rank 5
				prom_rank_mask = C64(0xFF); // rank 1
			}
			u64 noprom_ranks_mask = ~prom_rank_mask;

			Bitboard empty = ~pos.Occupancy();

			Bitboard single_push_targets = bitboard::StepOne<push_dir>(ally_pawns) & empty;
			Bitboard double_push_targets = bitboard::StepOne<push_dir>(single_push_targets) & empty & dpush_rank_mask & legal_squares;
			single_push_targets &= legal_squares;

			Bitboard promotion_targets = single_push_targets & prom_rank_mask;
			single_push_targets &= noprom_ranks_mask; // exclude promotion targets

			while (single_push_targets) {
				Square to = bitboard::BitScanForward(single_push_targets);
				Square from = to - push_dir;

				AddPawnPush(from, to);
				single_push_targets &= single_push_targets - 1;
			}

			while (double_push_targets) {
				Square to = bitboard::BitScanForward(double_push_targets);
				Square from = to - (2 * push_dir);

				AddDoublePawnPush(from, to);
				double_push_targets &= double_push_targets - 1;
			}

			while (promotion_targets) {
				Square to = bitboard::BitScanForward(promotion_targets);
				Square from = to - push_dir;

				AddPromotions(from, to);
				promotion_targets &= promotion_targets - 1;
			}
		}

		template <int side>
		void GeneratePawnCaptures(const GameState& pos, Bitboard ally_pawns, Bitboard opp_pieces, Bitboard capturable_squares)
		{
			constexpr int pawn_east_dir = direction::NE - (side << 4); // South east if black
			constexpr int pawn_west_dir = direction::NW - (side << 4); // South west if black
			u64 prom_rank_mask;
			if constexpr (side == side::WHITE) {
				prom_rank_mask = C64(0xFF00000000000000); // rank 8
			}
			else {
				prom_rank_mask = C64(0xFF); // rank 1
			}
			u64 noprom_ranks_mask = ~prom_rank_mask;

			Bitboard pawn_east_targets = bitboard::StepOne<pawn_east_dir>(ally_pawns) & opp_pieces & capturable_squares;
			Bitboard pawn_east_targets_prom = pawn_east_targets & prom_rank_mask;
			pawn_east_targets &= noprom_ranks_mask;

			while (pawn_east_targets) {
				Square to = bitboard::BitScanForward(pawn_east_targets);
				Square from = to - pawn_east_dir;
				PieceType captured_piece = pos.GetPiece(to);

				AddCapture<piece_type::PAWN>(from, to, captured_piece);
				pawn_east_targets &= pawn_east_targets - 1;
			}

			while (pawn_east_targets_prom) {
				Square to = bitboard::BitScanForward(pawn_east_targets_prom);
				Square from = to - pawn_east_dir;
				PieceType captured_piece = pos.GetPiece(to);

				AddCapturePromotions(from, to, captured_piece);
				pawn_east_targets_prom &= pawn_east_targets_prom - 1;
			}

			Bitboard pawn_west_targets = bitboard::StepOne<pawn_west_dir>(ally_pawns) & opp_pieces & capturable_squares;
			Bitboard pawn_west_targets_prom = pawn_west_targets & prom_rank_mask;
			pawn_west_targets &= noprom_ranks_mask;


			while (pawn_west_targets) {
				Square to = bitboard::BitScanForward(pawn_west_targets);
				Square from = to - pawn_west_dir;
				PieceType captured_piece = pos.GetPiece(to);

				AddCapture<piece_type::PAWN>(from, to, captured_piece);
				pawn_west_targets &= pawn_west_targets - 1;
			}


			while (pawn_west_targets_prom) {
				Square to = bitboard::BitScanForward(pawn_west_targets_prom);
				Square from = to - pawn_west_dir;
				PieceType captured_piece = pos.GetPiece(to);

				AddCapturePromotions(from, to, captured_piece);
				pawn_west_targets_prom &= pawn_west_targets_prom - 1;
			}
		}

		template <int side>
		bool GenerateMoves(const GameState& pos, bool quiescence = false);

		template <int side>
		void GenerateCaptures(const GameState& pos);
	private:

		static constexpr int PromotablePieces[4] { piece_type::QUEEN, piece_type::BISHOP,  piece_type::ROOK, piece_type::KNIGHT };


		force_inline void AddPawnPush(Square from, Square to)
		{
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			Move m = from;
			m |= (to << 6);
			m |= (piece_type::PAWN << 23); // moving piece
	
			moves[length].move = m;
			moves[length].score = move::QUIET_SCORE;
			++length;

			ANKA_ASSERT(move::Validate(m, from, to, piece_type::PAWN));
		}

		force_inline void AddDoublePawnPush(Square from, Square to)
		{
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			Move m(from);
			m |= (to << 6);
			m |= (piece_type::PAWN << 23); // moving piece
			
			move::SetDoublePawnBit(m);

			moves[length].move= m;
			moves[length].score = move::QUIET_SCORE;
			++length;

			ANKA_ASSERT(move::Validate(m, from, to, piece_type::PAWN, 0, 0, 0, 1));
		}

		force_inline void AddPromotions(Square from, Square to)
		{
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			for (auto prom_piece : PromotablePieces) {
				Move m(from);
				m |= (to << 6);
				m |= (piece_type::PAWN << 23); // moving piece
				m |= (prom_piece << 20); // promoted piece

				move::SetPromotionBit(m);
				moves[length].move = m;
				moves[length].score = move::PROMOTION_SCORE;
				++length;
				ANKA_ASSERT(move::Validate(m, from, to, piece_type::PAWN,1,0,0,0,0,0,prom_piece));
			}
		}


		force_inline void AddCapturePromotions(Square from, Square to, PieceType captured_piece)
		{
			ANKA_ASSERT(captured_piece >= piece_type::PAWN && captured_piece < piece_type::KING);
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			for (auto prom_piece : PromotablePieces) {
				Move m(from);
				m |= (to << 6);
				m |= (piece_type::PAWN << 23); // moving piece
				m |= (prom_piece << 20); // promoted piece
				m |= (captured_piece << 17); // captured piece

				move::SetPromotionBit(m);
				move::SetCaptureBit(m);

				moves[length].move = m;
				moves[length].score = move::MVVLVA_CAPTURE_SCORES[piece_type::PAWN][captured_piece] + move::PROMOTION_SCORE;
				++length;
				ANKA_ASSERT(move::Validate(m, from, to, piece_type::PAWN, 1, 1, 0, 0, 0, captured_piece, prom_piece));
			}
		}

		template <int moving_piece>
		force_inline void AddCapture(Square from, Square to, PieceType captured_piece)
		{
			ANKA_ASSERT(captured_piece >= piece_type::PAWN && captured_piece < piece_type::KING);
			static_assert(moving_piece >= piece_type::PAWN && moving_piece <= piece_type::KING, "Invalid moving piece type");
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			Move m(from);
			m |= (to << 6);
			m |= (moving_piece << 23); // moving piece
			m |= (captured_piece << 17); // captured piece

			move::SetCaptureBit(m);

			moves[length].move = m;
			moves[length].score = move::MVVLVA_CAPTURE_SCORES[moving_piece][captured_piece];
			++length;

			ANKA_ASSERT(move::Validate(m, from, to, moving_piece, 0, 1, 0, 0, 0, captured_piece, 0));
		}


		force_inline void AddEpCapture(Square from, Square to)
		{
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			Move m(from);
			m |= (to << 6);
			m |= (piece_type::PAWN << 23); // moving piece
			m |= (piece_type::PAWN << 17); // captured piece

			move::SetCaptureBit(m);
			move::SetEpCaptureBit(m);

			moves[length].move = m;
			moves[length].score = move::MVVLVA_CAPTURE_SCORES[piece_type::PAWN][piece_type::PAWN];
			++length;

			ANKA_ASSERT(move::Validate(m, from, to, piece_type::PAWN, 0, 1, 0, 0, 1, piece_type::PAWN, 0));
		}

		template <int moving_piece>
		force_inline void AddQuiet(Square from, Square to)
		{
			static_assert(moving_piece >= piece_type::PAWN && moving_piece <= piece_type::KING, "Invalid moving piece type");
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			Move m = from;
			m |= (to << 6);
			m |= (moving_piece << 23); // moving piece

			moves[length].move = m;
			moves[length].score = move::QUIET_SCORE;
			++length;

			ANKA_ASSERT(move::Validate(m, from, to, moving_piece));
		}

		force_inline void AddCastle(Square from, Square to)
		{
			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			Move m = from;
			m |= (to << 6);
			m |= (piece_type::KING << 23); // moving piece

			move::SetCastleBit(m);

			moves[length].move = m;
			moves[length].score = move::CASTLE_SCORE;
			++length;

			ANKA_ASSERT(move::Validate(m, from, to, piece_type::KING,0,0,1));
		}
	};


	template<size_t n>
	force_inline bool MoveList<n>::GenerateLegalMoves(const GameState& pos, bool quiescence)
	{
		length = 0;

		if (pos.SideToPlay() == side::WHITE) {
			return GenerateMoves<side::WHITE>(pos, quiescence);
		}
		else if (pos.SideToPlay() == side::BLACK) {
			return GenerateMoves<side::BLACK>(pos, quiescence);
		}
		else
			return false;

	}

	template<size_t n>
	inline void MoveList<n>::GenerateLegalCaptures(const GameState& pos)
	{
		length = 0;

		if (pos.SideToPlay() == side::WHITE) {
			GenerateCaptures<side::WHITE>(pos);
		}
		else if (pos.SideToPlay() == side::BLACK) {
			GenerateCaptures<side::BLACK>(pos);
		}
	}



	template<size_t n>
	template<int side>
	inline void MoveList<n>::GenerateCaptures(const GameState& pos)
	{
		using namespace piece_type;
		static_assert(side == side::WHITE || side == side::BLACK, "Side must be white or black when generating moves.");

		constexpr int ally_side(side);
		constexpr int opponent_side(side ^ 1);

		Bitboard occ = pos.Occupancy();
		Bitboard empty = ~occ;
		Bitboard ally_pieces = pos.Pieces<ally_side>();
		Bitboard opp_pieces = pos.Pieces<opponent_side>();
		Bitboard opp_knights = opp_pieces & pos.Knights();
		Bitboard opp_queens = opp_pieces & pos.Queens();
		Bitboard opp_kings = opp_pieces & pos.Kings();
		Bitboard opp_bishops_and_queens = (opp_pieces & pos.Bishops()) | opp_queens;
		Bitboard opp_rooks_and_queens = (opp_pieces & pos.Rooks()) | opp_queens;
		Bitboard opp_pawns = opp_pieces & pos.Pawns();

		// DETERMINE CHECKERS
		Bitboard ally_king = pos.Pieces<ally_side, KING>();
		Square king_sq = bitboard::BitScanForward(ally_king);

		Bitboard rook_attacks_king_sq = attacks::RookAttacks(king_sq, occ);
		Bitboard bishop_attacks_king_sq = attacks::BishopAttacks(king_sq, occ);

		Bitboard cross_checkers = rook_attacks_king_sq & opp_rooks_and_queens;
		Bitboard diagonal_checkers = bishop_attacks_king_sq & opp_bishops_and_queens;
		Bitboard checkers = (attacks::KnightAttacks(king_sq) & opp_knights)
			| cross_checkers | diagonal_checkers
			| (attacks::PawnAttacks<ally_side>(king_sq) & opp_pawns);

		int num_checkers = bitboard::PopCount(checkers);


		// GENERATE LEGAL KING CAPTURES
		Bitboard king_attacks = attacks::KingAttacks(king_sq);
		Bitboard king_targets = king_attacks & opp_pieces;

		while (king_targets) {
			Square sq = bitboard::BitScanForward(king_targets);
			king_targets &= king_targets - 1;

			// check if target square is under attack
			if (attacks::KnightAttacks(sq) & opp_knights)
				continue;
			else if (attacks::BishopAttacks(sq, occ ^ ally_king) & opp_bishops_and_queens)
				continue;
			else if (attacks::RookAttacks(sq, occ ^ ally_king) & opp_rooks_and_queens)
				continue;
			else if (attacks::PawnAttacks<ally_side>(sq) & opp_pawns)
				continue;
			else if (attacks::KingAttacks(sq) & opp_kings)
				continue;
			else
				AddCapture<piece_type::KING>(king_sq, sq, pos.GetPiece(sq));
		}
	

		if (num_checkers > 1) // double check
			return;



		Bitboard capturable_squares = UINT64_MAX;
		Bitboard pushable_squares = C64(0);

		// thanks to https://peterellisjones.com/posts/generating-legal-chess-moves-efficiently/
		if (num_checkers == 1) { // single check
			// capturing checker
			capturable_squares = checkers;
		}

		// CALCULATE ABSOLUTELY PINNED PIECES
		Bitboard absolute_pins = C64(0);

		Bitboard cross_blockers = (ally_pieces & rook_attacks_king_sq);
		Bitboard cross_pinners = rook_attacks_king_sq ^ attacks::RookAttacks(king_sq, occ ^ cross_blockers);
		cross_pinners &= opp_rooks_and_queens;

		Bitboard diagonal_blockers = (ally_pieces & bishop_attacks_king_sq);
		Bitboard diagonal_pinners = bishop_attacks_king_sq ^ attacks::BishopAttacks(king_sq, occ ^ diagonal_blockers);
		diagonal_pinners &= opp_bishops_and_queens;

		while (cross_pinners) {
			Square pinner_sq = bitboard::BitScanForward(cross_pinners);
			Bitboard legal_squares = attacks::InBetween(pinner_sq, king_sq) | (C64(1) << pinner_sq);
			Bitboard pinned_piece_bb = legal_squares & ally_pieces;
			Square pinned_sq = bitboard::BitScanForward(pinned_piece_bb);
			PieceType pinned_type = pos.GetPiece(pinned_sq);

			if (pinned_type == ROOK) {
				GeneratePieceMoves<side, ROOK>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == QUEEN) {
				GeneratePieceMoves<side, QUEEN>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == PAWN) {
				GeneratePawnPushMoves<side>(pos, pinned_piece_bb, legal_squares & pushable_squares);
			}
			absolute_pins |= pinned_piece_bb;
			cross_pinners &= cross_pinners - 1;
		}

		while (diagonal_pinners) {
			Square pinner_sq = bitboard::BitScanForward(diagonal_pinners);
			Bitboard legal_squares = attacks::InBetween(pinner_sq, king_sq) | (C64(1) << pinner_sq);
			Bitboard pinned_piece_bb = legal_squares & ally_pieces;
			Square pinned_sq = bitboard::BitScanForward(pinned_piece_bb);
			PieceType pinned_type = pos.GetPiece(pinned_sq);

			if (pinned_type == BISHOP) {
				GeneratePieceMoves<side, BISHOP>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == QUEEN) {
				GeneratePieceMoves<side, QUEEN>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == PAWN) {
				GeneratePawnCaptures<side>(pos, pinned_piece_bb, opp_pieces, legal_squares & capturable_squares);
			}
			absolute_pins |= pinned_piece_bb;
			diagonal_pinners &= diagonal_pinners - 1;
		}

		Bitboard free_to_move = ~absolute_pins;

		/****** PAWN MOVES *****/
		Bitboard ally_pawns = ally_pieces & pos.Pawns() & free_to_move;
		GeneratePawnCaptures<side>(pos, ally_pawns, opp_pieces, capturable_squares);

		// EP CAPTURE
		Square ep_square = pos.EnPassantSquare();
		if (ep_square != square::NOSQUARE) {
			constexpr Rank ep_rank = rank::FIVE - side; //
			constexpr int ep_capture_target_dir = direction::S + (side << 4); // south if white, north if black
			Square ep_capture_target = ep_square + ep_capture_target_dir;

			bool ep_is_possible = bitboard::BitIsSet(pushable_squares, ep_square) || bitboard::BitIsSet(capturable_squares, ep_capture_target);

			if (ep_is_possible) {
				Bitboard ep_attackers = attacks::PawnAttacks<opponent_side>(ep_square) & ally_pawns;

				while (ep_attackers) {
					Square sq = bitboard::BitScanForward(ep_attackers);
					ep_attackers &= ep_attackers - 1;

					// check if ep capture results in a check
					if (square::GetRank(king_sq) == ep_rank) {
						// xray attack through involved pawns
						u64 blockers = (C64(1) << sq) | (C64(1) << ep_capture_target);
						Bitboard xray_attacks = rook_attacks_king_sq ^ attacks::RookAttacks(king_sq, occ ^ blockers);
						if (xray_attacks & opp_rooks_and_queens)
							continue;
					}
					AddEpCapture(sq, ep_square);
				}
			}
		}


		// KNIGHTS AND SLIDERS
		GeneratePieceMoves<side, KNIGHT>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
		GeneratePieceMoves<side, BISHOP>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
		GeneratePieceMoves<side, ROOK>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
		GeneratePieceMoves<side, QUEEN>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
	}



	template<size_t n>
	template<int side>
	inline bool MoveList<n>::GenerateMoves(const GameState& pos, bool quiescence)
	{
		using namespace piece_type;
		static_assert(side == side::WHITE || side == side::BLACK, "Side must be white or black when generating moves.");

		constexpr int ally_side(side);
		constexpr int opponent_side(side ^ 1);

		Bitboard occ = pos.Occupancy();
		Bitboard empty = ~occ;
		Bitboard ally_pieces = pos.Pieces<ally_side>();
		Bitboard opp_pieces = pos.Pieces<opponent_side>();
		Bitboard opp_knights = opp_pieces & pos.Knights();
		Bitboard opp_queens = opp_pieces & pos.Queens();
		Bitboard opp_kings = opp_pieces & pos.Kings();
		Bitboard opp_bishops_and_queens = (opp_pieces & pos.Bishops()) | opp_queens;
		Bitboard opp_rooks_and_queens = (opp_pieces & pos.Rooks()) | opp_queens;
		Bitboard opp_pawns = opp_pieces & pos.Pawns();


		// DETERMINE CHECKERS
		Bitboard ally_king = pos.Pieces<ally_side, KING>();
		Square king_sq = bitboard::BitScanForward(ally_king);

		Bitboard rook_attacks_king_sq = attacks::RookAttacks(king_sq, occ);
		Bitboard bishop_attacks_king_sq = attacks::BishopAttacks(king_sq, occ);

		Bitboard cross_checkers = rook_attacks_king_sq & opp_rooks_and_queens;
		Bitboard diagonal_checkers = bishop_attacks_king_sq & opp_bishops_and_queens;
		Bitboard checkers = (attacks::KnightAttacks(king_sq) & opp_knights)
			| cross_checkers | diagonal_checkers
			| (attacks::PawnAttacks<ally_side>(king_sq) & opp_pawns);

		int num_checkers = bitboard::PopCount(checkers);

		if (!quiescence || num_checkers > 0) {
			// GENERATE LEGAL KING MOVES
			Bitboard king_attacks = attacks::KingAttacks(king_sq);
			Bitboard king_targets = king_attacks & opp_pieces;
			Bitboard king_push_targets = king_attacks & empty;

			while (king_targets) {
				Square sq = bitboard::BitScanForward(king_targets);
				king_targets &= king_targets - 1;

				// check if target square is under attack
				// BUG: slider attacks need to xray through king
				if (attacks::KnightAttacks(sq) & opp_knights)
					continue;
				else if (attacks::BishopAttacks(sq, occ ^ ally_king) & opp_bishops_and_queens)
					continue;
				else if (attacks::RookAttacks(sq, occ ^ ally_king) & opp_rooks_and_queens)
					continue;
				else if (attacks::PawnAttacks<ally_side>(sq) & opp_pawns)
					continue;
				else if (attacks::KingAttacks(sq) & opp_kings)
					continue;
				else
					AddCapture<piece_type::KING>(king_sq, sq, pos.GetPiece(sq));
			}

			while (king_push_targets) {
				Square sq = bitboard::BitScanForward(king_push_targets);
				king_push_targets &= king_push_targets - 1;

				// check if target square is under attack
				if (attacks::KnightAttacks(sq) & opp_knights)
					continue;
				else if (attacks::BishopAttacks(sq, occ ^ ally_king) & opp_bishops_and_queens)
					continue;
				else if (attacks::RookAttacks(sq, occ ^ ally_king) & opp_rooks_and_queens)
					continue;
				else if (attacks::PawnAttacks<ally_side>(sq) & opp_pawns)
					continue;
				else if (attacks::KingAttacks(sq) & opp_kings)
					continue;
				else
					AddQuiet<piece_type::KING>(king_sq, sq);
			}
		}

		

		if (num_checkers > 1) // double check
			return true;

		

		Bitboard capturable_squares = UINT64_MAX;
		Bitboard pushable_squares = UINT64_MAX;

		// thanks to https://peterellisjones.com/posts/generating-legal-chess-moves-efficiently/
		if (num_checkers == 1) { // single check
			// capturing checker
			capturable_squares = checkers;

			// interposing with another piece
			Square checker_sq = bitboard::BitScanForward(checkers);
			PieceType checker_piece = pos.GetPiece(checker_sq);
			if (piece_type::IsSlider(checker_piece)) {
				pushable_squares = attacks::InBetween(checker_sq, king_sq);
			}
			else {
				pushable_squares = C64(0);
			}
		}
		else {
			if (quiescence) {
				pushable_squares = C64(0);
			}
			else {
				// castling
				GenerateCastleMoves<side>(pos);
			}
		}


		// CALCULATE ABSOLUTELY PINNED PIECES
		Bitboard absolute_pins = C64(0);

		Bitboard cross_blockers = (ally_pieces & rook_attacks_king_sq);
		Bitboard cross_pinners = rook_attacks_king_sq ^ attacks::RookAttacks(king_sq, occ ^ cross_blockers);
		cross_pinners &= opp_rooks_and_queens;

		Bitboard diagonal_blockers = (ally_pieces & bishop_attacks_king_sq);
		Bitboard diagonal_pinners = bishop_attacks_king_sq ^ attacks::BishopAttacks(king_sq, occ ^ diagonal_blockers);
		diagonal_pinners &= opp_bishops_and_queens;

		while (cross_pinners) {
			Square pinner_sq = bitboard::BitScanForward(cross_pinners);
			Bitboard legal_squares = attacks::InBetween(pinner_sq, king_sq) | (C64(1) << pinner_sq);
			Bitboard pinned_piece_bb = legal_squares & ally_pieces;
			Square pinned_sq = bitboard::BitScanForward(pinned_piece_bb);
			PieceType pinned_type = pos.GetPiece(pinned_sq); 

			if (pinned_type == ROOK) {
				GeneratePieceMoves<side, ROOK>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == QUEEN) {
				GeneratePieceMoves<side, QUEEN>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == PAWN) {
				GeneratePawnPushMoves<side>(pos, pinned_piece_bb, legal_squares & pushable_squares);
			}
			absolute_pins |= pinned_piece_bb;
			cross_pinners &= cross_pinners - 1;
		}

		while (diagonal_pinners) {
			Square pinner_sq = bitboard::BitScanForward(diagonal_pinners);
			Bitboard legal_squares = attacks::InBetween(pinner_sq, king_sq) | (C64(1) << pinner_sq);
			Bitboard pinned_piece_bb = legal_squares & ally_pieces;
			Square pinned_sq = bitboard::BitScanForward(pinned_piece_bb);
			PieceType pinned_type = pos.GetPiece(pinned_sq);

			if (pinned_type == BISHOP) {
				GeneratePieceMoves<side, BISHOP>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == QUEEN) {
				GeneratePieceMoves<side, QUEEN>(pos, pinned_piece_bb, opp_pieces, UINT64_MAX, legal_squares & pushable_squares, legal_squares & capturable_squares);
			}
			else if (pinned_type == PAWN) {
				GeneratePawnCaptures<side>(pos, pinned_piece_bb, opp_pieces, legal_squares & capturable_squares);
			}
			absolute_pins |= pinned_piece_bb;
			diagonal_pinners &= diagonal_pinners - 1;
		}

		Bitboard free_to_move = ~absolute_pins;

		/****** PAWN MOVES *****/
		Bitboard ally_pawns = ally_pieces & pos.Pawns() & free_to_move;
		GeneratePawnPushMoves<side>(pos, ally_pawns, pushable_squares);
		GeneratePawnCaptures<side>(pos, ally_pawns, opp_pieces, capturable_squares);

		// EP CAPTURE
		Square ep_square = pos.EnPassantSquare();
		if (ep_square != square::NOSQUARE) {
			constexpr Rank ep_rank = rank::FIVE - side; //
			constexpr int ep_capture_target_dir = direction::S + (side << 4); // south if white, north if black
			Square ep_capture_target = ep_square + ep_capture_target_dir;

			bool ep_is_possible = bitboard::BitIsSet(pushable_squares, ep_square) || bitboard::BitIsSet(capturable_squares, ep_capture_target);

			if (ep_is_possible) {
				Bitboard ep_attackers = attacks::PawnAttacks<opponent_side>(ep_square) & ally_pawns;

				while (ep_attackers) {
					Square sq = bitboard::BitScanForward(ep_attackers);
					ep_attackers &= ep_attackers - 1;

					// check if ep capture results in a check
					if (square::GetRank(king_sq) == ep_rank) {
						// xray attack through involved pawns
						u64 blockers = (C64(1) << sq) | (C64(1) << ep_capture_target);
						Bitboard xray_attacks = rook_attacks_king_sq ^ attacks::RookAttacks(king_sq, occ ^ blockers);
						if (xray_attacks & opp_rooks_and_queens)
							continue;
					}
					AddEpCapture(sq, ep_square);
				}
			}
		}


		// KNIGHTS AND SLIDERS
		GeneratePieceMoves<side, KNIGHT>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
		GeneratePieceMoves<side, BISHOP>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
		GeneratePieceMoves<side, ROOK>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);
		GeneratePieceMoves<side, QUEEN>(pos, ally_pieces, opp_pieces, free_to_move, pushable_squares, capturable_squares);

		//GenerateNonPawnMoves(pos, ally_pieces, opp_pieces, occ, free_to_move, capturable_squares, pushable_squares);

		return (bool)num_checkers;
	}

	
	template<size_t n>
	inline void MoveList<n>::GenerateNonPawnMoves(const GameState& pos,
		Bitboard ally_pieces, Bitboard opp_pieces,
		Bitboard occ, Bitboard free_mask,
		Bitboard capturables, Bitboard pushables )
	{
		using namespace piece_type;

		Bitboard empty = ~occ;

		// KNIGHT MOVES
		Bitboard ally_knights = ally_pieces & pos.Knights() & free_mask;

		while (ally_knights) {
			Square from = bitboard::BitScanForward(ally_knights);
			Bitboard attacks = attacks::KnightAttacks(from);
			Bitboard cap_targets = attacks & opp_pieces & capturables;
			Bitboard push_targets = attacks & empty & pushables;
			while (cap_targets) {
				Square to = bitboard::BitScanForward(cap_targets);
				AddCapture<KNIGHT>(from, to, pos.GetPiece(to));
				cap_targets &= cap_targets - 1;
			}
			while (push_targets) {
				Square to = bitboard::BitScanForward(push_targets);
				AddQuiet<KNIGHT>(from, to);
				push_targets &= push_targets - 1;
			}
			ally_knights &= ally_knights - 1;
		}

		// ROOK MOVES
		Bitboard ally_rooks = ally_pieces & pos.Rooks() & free_mask;

		while (ally_rooks) {
			Square from = bitboard::BitScanForward(ally_rooks);
			Bitboard attacks = attacks::RookAttacks(from, occ);
			Bitboard cap_targets = attacks & opp_pieces & capturables;
			Bitboard push_targets = attacks & empty & pushables;
			while (cap_targets) {
				Square to = bitboard::BitScanForward(cap_targets);
				AddCapture<ROOK>(from, to, pos.GetPiece(to));
				cap_targets &= cap_targets - 1;
			}
			while (push_targets) {
				Square to = bitboard::BitScanForward(push_targets);
				AddQuiet<ROOK>(from, to);
				push_targets &= push_targets - 1;
			}
			ally_rooks &= ally_rooks - 1;
		}

		// BISHOP MOVES
		Bitboard ally_bishops = ally_pieces & pos.Bishops() & free_mask;

		while (ally_bishops) {
			Square from = bitboard::BitScanForward(ally_bishops);
			Bitboard attacks = attacks::BishopAttacks(from, occ);
			Bitboard cap_targets = attacks & opp_pieces & capturables;
			Bitboard push_targets = attacks & empty & pushables;
			while (cap_targets) {
				Square to = bitboard::BitScanForward(cap_targets);
				AddCapture<BISHOP>(from, to, pos.GetPiece(to));
				cap_targets &= cap_targets - 1;
			}
			while (push_targets) {
				Square to = bitboard::BitScanForward(push_targets);
				AddQuiet<BISHOP>(from, to);
				push_targets &= push_targets - 1;
			}
			ally_bishops &= ally_bishops - 1;
		}

		// QUEEN MOVES
		Bitboard ally_queens = ally_pieces & pos.Queens() & free_mask;

		while (ally_queens) {
			Square from = bitboard::BitScanForward(ally_queens);
			Bitboard attacks = attacks::QueenAttacks(from, occ);
			Bitboard cap_targets = attacks & opp_pieces & capturables;
			Bitboard push_targets = attacks & empty & pushables;
			while (cap_targets) {
				Square to = bitboard::BitScanForward(cap_targets);
				AddCapture<QUEEN>(from, to, pos.GetPiece(to));
				cap_targets &= cap_targets - 1;
			}
			while (push_targets) {
				Square to = bitboard::BitScanForward(push_targets);
				AddQuiet<QUEEN>(from, to);
				push_targets &= push_targets - 1;
			}
			ally_queens &= ally_queens - 1;
		}
	}

	template<size_t n>
	inline void MoveList<n>::PrintMoves() const
	{
		char move_str[6];
		for (int i = 0; i < length; i++) {
			Move m = moves[i].move;
			int score = moves[i].score;
			
			move::ToString(m, move_str);

			printf("%d: {%s, %d}\n", i + 1, move_str, score);
		}
	}
} // namespace anka