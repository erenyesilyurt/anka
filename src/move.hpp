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
		enum {NO_MOVE = 0, NULL_MOVE = 0xffffffff};


		inline constexpr int QUIET_SCORE = 0;
		inline constexpr int HASH_MOVE_SCORE = 500000;
		inline constexpr int KILLER_SCORE = 10000;
		inline constexpr int CAPTURE_SCORE = 11000;		
		

		inline constexpr int MVVLVA[8][8] = { // [attacker][victim]
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
			PieceType cap_piece = NO_PIECE, PieceType prom_piece = NO_PIECE);

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
		int ToString(Move move, char* result);
	};
}