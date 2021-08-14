#pragma once

#include "core.hpp"
#include "lut.hpp"

namespace anka {
	enum CastlePermFlags_t { castle_wk = 8, castle_wq = 4, castle_bk = 2, castle_bq = 1 };
	inline constexpr int MATERIAL_VALUES[8] = { 0, 0, 100, 300, 300, 500, 900, 0 };

	typedef int Rank;
	typedef int File;
	typedef int Square;
	typedef int PieceType;
	typedef int Side;
	typedef int Square;

	// Directions
	enum Direction { EAST = 1, WEST = -1, NORTH = 8, NORTHEAST = 9, NORTHWEST = 7, SOUTH = -8, SOUTHEAST = -7, SOUTHWEST = -9 };
	
	// Game phase
	enum Phase {MIDGAME, ENDGAME};

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
		enum { NOPIECE = 0, PAWN = 2, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL };

		force_inline bool IsSlider(PieceType p)
		{
			return lut::LUT_ISSLIDER[p];
		}

		force_inline PieceType CharToPieceType(char c)
		{
			switch (c) {
			case 'p':
			case 'P':
				return PAWN;
			case 'n':
			case 'N':
				return KNIGHT;
			case 'b':
			case 'B':
				return BISHOP;
			case 'r':
			case 'R':
				return ROOK;
			case 'q':
			case 'Q':
				return QUEEN;
			case 'k':
			case 'K':
				return KING;
			default:
				return NOPIECE;
			}
		}
	};

	namespace side {

		enum { WHITE = 0, BLACK = 1, ALL = 2, NONE = 2 };

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