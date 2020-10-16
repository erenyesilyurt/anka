#pragma once
#include "boarddefs.hpp"
#include <assert.h>

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
	typedef uint32_t Move;

	struct GradedMove {
		Move move;
		int score;
	};


	namespace move {

		inline constexpr int QUIET_SCORE = 0;
		inline constexpr int CASTLE_SCORE = 100;
		inline constexpr int PROMOTION_SCORE = 210;
		inline constexpr int PV_SCORE = 25000;
		inline constexpr int KILLER_SCORE = 195;

		inline constexpr int MVVLVA_CAPTURE_SCORES[8][8] = { // [attacker][victim]
			{0, 0,  0,  0,  0,  0,  0,  0}, // NOPIECE X x
			{0, 0,  0,  0,  0,  0,  0,  0}, // NOPIECE X x
			{0, 0, 200, 400, 500, 800, 1000, 0}, // pawn X x
			{0, 0, 190, 390, 490, 790, 990, 0}, // knight X x
			{0, 0, 180, 380, 480, 780, 980, 0}, // bishop X x
			{0, 0, 170, 370, 470, 770, 970, 0}, // rook X x
			{0, 0, 160, 360, 460, 760, 960, 0}, // queen X x
			{0, 0, 150, 350, 450, 750, 950, 0}, // king X x
		};

		bool Validate(Move m, Square from, Square to, PieceType moving_piece, 
			bool is_prom = false, bool is_capture = false, bool is_castle = false, 
			bool is_dpush = false, bool is_epcapture = false, 
			PieceType cap_piece = piece_type::NOPIECE, PieceType prom_piece = piece_type::NOPIECE);

		struct MoveMasks {
			static constexpr u32 FROM_SQ = 0x3f;
			static constexpr u32 TO_SQ = 0xfc0;
			static constexpr u32 FLAGS = 0x1f000;
			static constexpr u32 PROMOTION_FLAG = 0x1000;
			static constexpr u32 CAPTURE_FLAG = 0x2000;
			static constexpr u32 CASTLE_FLAG = 0x4000;
			static constexpr u32 DOUBLE_PUSH_FLAG = 0x8000;
			static constexpr u32 EP_CAPTURE_FLAG = 0x10000;
			static constexpr u32 CAPTURED_PIECE = (1 << 17) | (1 << 18) | (1 << 19);
			static constexpr u32 PROMOTED_PIECE = (1 << 20) | (1 << 21) | (1 << 22);
			static constexpr u32 MOVING_PIECE = (1 << 23) | (1 << 24) | (1 << 25);
		};


		force_inline Square FromSquare(Move move)
		{
			return (Square)(move & MoveMasks::FROM_SQ);
		}

		force_inline Square ToSquare(Move move)
		{
			return (move & MoveMasks::TO_SQ) >> 6;
		}

		force_inline bool IsPromotion(Move move)
		{
			return move & MoveMasks::PROMOTION_FLAG;
		}

		force_inline bool IsCapture(Move move)
		{
			return move & MoveMasks::CAPTURE_FLAG;
		}

		force_inline bool IsCastle(Move move)
		{
			return move & MoveMasks::CASTLE_FLAG;
		}

		force_inline bool IsEpCapture(Move move)
		{
			return move & MoveMasks::EP_CAPTURE_FLAG;
		}

		force_inline bool IsDoublePawnPush(Move move)
		{

			return move & MoveMasks::DOUBLE_PUSH_FLAG;
		}

		force_inline bool IsQuiet(Move move)
		{
			return !(move & 0x13000);
		}

		force_inline PieceType CapturedPiece(Move move)
		{
			return (PieceType)((move & MoveMasks::CAPTURED_PIECE) >> 17);
		}

		force_inline PieceType PromotedPiece(Move move)
		{
			return (PieceType)((move & MoveMasks::PROMOTED_PIECE) >> 20);
		}

		force_inline PieceType MovingPiece(Move move)
		{
			return (PieceType)((move & MoveMasks::MOVING_PIECE) >> 23);
		}

		force_inline void SetPromotionBit(Move& move)
		{
			move |= (1 << 12);
		}

		force_inline void SetCaptureBit(Move& move)
		{
			move |= (1 << 13);
		}

		force_inline void SetCastleBit(Move& move)
		{
			move |= (1 << 14);
		}

		force_inline void SetDoublePawnBit(Move& move)
		{
			move |= (1 << 15);
		}

		force_inline void SetEpCaptureBit(Move& move)
		{
			move |= (1 << 16);
		}

		// returns string length
		inline int ToString(Move move, char *result)
		{
			int from_file = square::GetFile(FromSquare(move));
			int from_rank = square::GetRank(FromSquare(move));
			int to_file = square::GetFile(ToSquare(move));
			int to_rank = square::GetRank(ToSquare(move));
			int str_len = 4;
			PieceType promoted = piece_type::NOPIECE;
			if (IsPromotion(move)) {
				promoted = PromotedPiece(move);
			}

			result[0] = ('a' + from_file);
			result[1] = ('1' + from_rank);
			result[2] = ('a' + to_file);
			result[3] = ('1' + to_rank);
			result[4] = '\0';

			if (promoted != piece_type::NOPIECE) {
				char piece = 'q';
				if (promoted == piece_type::KNIGHT) {
					piece = 'n';
				}
				else if (promoted == piece_type::ROOK) {
					piece = 'r';
				}
				else if (promoted == piece_type::BISHOP) {
					piece = 'b';
				}

				result[4] = piece;
				result[5] = '\0';
				str_len = 5;
			}

			return str_len;
		}


	};
}