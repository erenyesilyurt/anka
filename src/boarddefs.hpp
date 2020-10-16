#pragma once

#include "core.hpp"

namespace anka {
	/* Look-up tables */
	namespace lut {
		// enum { NOPIECE = 0, PAWN = 2, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL };
		inline constexpr bool LUT_ISSLIDER[9] =
		{
			false, false, false, false, true, true, true, false, false
		};
		inline constexpr byte LUT_ISOFFBOARD[120] =
		{ 1,1,1,1,1,1,1,1,1,1,
		 1,1,1,1,1,1,1,1,1,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,0,0,0,0,0,0,0,0,1,
		 1,1,1,1,1,1,1,1,1,1,
		 1,1,1,1,1,1,1,1,1,1 };

		inline constexpr byte LUT_SQ120[64] =
		{
			21,22,23,24,25,26,27,28,
			31,32,33,34,35,36,37,38,
			41,42,43,44,45,46,47,48,
			51,52,53,54,55,56,57,58,
			61,62,63,64,65,66,67,68,
			71,72,73,74,75,76,77,78,
			81,82,83,84,85,86,87,88,
			91,92,93,94,95,96,97,98
		};

		inline constexpr int LUT_SQ64[120] =
		{ 64,64,64,64,64,64,64,64,64,64,
		 64,64,64,64,64,64,64,64,64,64,
		 64,0,1,2,3,4,5,6,7,64,
		 64,8,9,10,11,12,13,14,15,64,
		 64,16,17,18,19,20,21,22,23,64,
		 64,24,25,26,27,28,29,30,31,64,
		 64,32,33,34,35,36,37,38,39,64,
		 64,40,41,42,43,44,45,46,47,64,
		 64,48,49,50,51,52,53,54,55,64,
		 64,56,57,58,59,60,61,62,63,64,
		 64,64,64,64,64,64,64,64,64,64,
		 64,64,64,64,64,64,64,64,64,64 };

		// Square -> string constant mappings for IO
		inline constexpr const char* LUT_SQUARE_STR[65] = {
		"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
		"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
		"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
		"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
		"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
		"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
		"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
		"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "--"
		};

		inline constexpr const char* LUT_CASTLING_RIGHTS_STR[16] = {
			"--", "q", "k", "kq",
			"Q", "Qq", "Qk", "Qkq",
			"K", "Kq", "Kk", "Kkq",
			"KQ", "KQq", "KQk", "KQkq"
		};

		inline constexpr const char* LUT_SIDE_STR[3] = {
			"White", "Black", "--"
		};
		inline constexpr const char* LUT_FILE_STR[9] = {
			"A", "B", "C", "D", "E", "F", "G", "H", "None"
		};
		inline constexpr const char* LUT_RANK_STR[9] = {
			"1", "2", "3", "4", "5", "6", "7", "8", "None"
		};
	} // namespace lut

	enum CastlePermFlags_t { castle_wk = 8, castle_wq = 4, castle_bk = 2, castle_bq = 1 };
	inline constexpr int MATERIAL_VALUES[8] = { 0, 0, 100, 300, 300, 500, 900, 0 };

	typedef int Rank;
	typedef int File;
	typedef int Square;
	typedef int PieceType;
	typedef int Side;
	typedef int Square;

	namespace direction {
		enum { E = 1, W = -1, N = 8, NE = 9, NW = 7, S = -8, SE = -7, SW = -9 };
	}

	
	namespace rank {
		force_inline const char* ToString(Rank r)
		{
			return lut::LUT_RANK_STR[r];
		}

		enum { ONE = 0, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NONE };
	};

	namespace file {
		enum { A = 0, B, C, D, E, F, G, H, NONE };

		force_inline const char* ToString(File f)
		{
			return lut::LUT_FILE_STR[f];
		}
	};

	namespace piece_type {
		enum { NOPIECE = 0, PAWN = 2, KNIGHT, BISHOP, ROOK, QUEEN, KING, LOWERBOUND };

		force_inline bool IsSlider(PieceType p)
		{
			return lut::LUT_ISSLIDER[p];
		}
	};

	namespace side {

		enum { WHITE = 0, BLACK = 1, LOWERBOUND = 2 };

		force_inline const char* ToString(Side s)
		{
			return lut::LUT_SIDE_STR[s];
		}
	};

	namespace square {
		// Least endian mapping, A1 = 0, B1 = 1, ... H8 = 63
		enum
		{
			A1, B1, C1, D1, E1, F1, G1, H1,
			A2, B2, C2, D2, E2, F2, G2, H2,
			A3, B3, C3, D3, E3, F3, G3, H3,
			A4, B4, C4, D4, E4, F4, G4, H4,
			A5, B5, C5, D5, E5, F5, G5, H5,
			A6, B6, C6, D6, E6, F6, G6, H6,
			A7, B7, C7, D7, E7, F7, G7, H7,
			A8, B8, C8, D8, E8, F8, G8, H8, NOSQUARE
		};

		
		force_inline Rank GetRank(Square sq)
		{
			return sq >> 3;
		}

		force_inline File GetFile(Square sq)
		{
			return sq & 7;
		}

		force_inline Square RankFileToSquare(Rank r, File f)
		{
			return (r << 3) + f;
		}

		force_inline const char* ToString(Square sq)
		{
			return lut::LUT_SQUARE_STR[sq];
		}

		// given a bit index (0..63), returns a square index in the 12x10 board
		force_inline int Square64To120(Square index64)
		{
			return lut::LUT_SQ120[index64];
		}

		// given a square index in the 12x10 board, returns a bit index (0..63)
		force_inline Square Square120To64(int index120)
		{
			return lut::LUT_SQ64[index120];
		}

		// given a 12x10 square, checks whether it is a valid square
		force_inline bool IsOffBoard120(int sq120)
		{
			return lut::LUT_ISOFFBOARD[sq120];
		}
	};




	force_inline const char* CastlingRightsToString(byte castling_rights)
	{
		return lut::LUT_CASTLING_RIGHTS_STR[castling_rights];
	}



} // namespace anka