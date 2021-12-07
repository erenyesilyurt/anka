#pragma once

#include "core.hpp"
#include "lut.hpp"

namespace anka {
	enum CastlePermFlags_t { castle_wk = 8, castle_wq = 4, castle_bk = 2, castle_bq = 1 };

	typedef int Rank;
	typedef int File;
	typedef int PieceType;
	typedef int Side;
	typedef int Square;

	enum { WHITE = 0, BLACK = 1, NUM_SIDES = 2, NOSIDE = 2, ALLSIDES = 2 }; // Side
	enum { RANK_ONE = 0, RANK_TWO, RANK_THREE, RANK_FOUR, RANK_FIVE, RANK_SIX, RANK_SEVEN, RANK_EIGHT, RANK_NONE }; // Rank
	enum { FILE_A = 0, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE }; // File
	enum { NO_PIECE = 0, PAWN = 2, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES }; // Piece Type (STARTS FROM PAWN = 2)
	enum Direction { EAST = 1, WEST = -1, NORTH = 8, NORTHEAST = 9, NORTHWEST = 7, SOUTH = -8, SOUTHEAST = -7, SOUTHWEST = -9 };
	enum Phase {MG, EG, NUM_PHASES};

	// Least endian mapping, A1 = 0, B1 = 1, ... H8 = 63
	enum // Square
	{
		A1, B1, C1, D1, E1, F1, G1, H1,
		A2, B2, C2, D2, E2, F2, G2, H2,
		A3, B3, C3, D3, E3, F3, G3, H3,
		A4, B4, C4, D4, E4, F4, G4, H4,
		A5, B5, C5, D5, E5, F5, G5, H5,
		A6, B6, C6, D6, E6, F6, G6, H6,
		A7, B7, C7, D7, E7, F7, G7, H7,
		A8, B8, C8, D8, E8, F8, G8, H8, NO_SQUARE
	};


	force_inline bool PieceIsSlider(PieceType p)
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
			return NO_PIECE;
		}
	}

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

	force_inline const char* SquareToString(Square sq)
	{
		return lut::LUT_SQUARE_STR[sq];
	}

	force_inline const char* RankToString(Rank r)
	{
		return lut::LUT_RANK_STR[r];
	}

	force_inline const char* FileToString(File f)
	{
		return lut::LUT_FILE_STR[f];
	}

	force_inline const char* SideToString(Side s)
	{
		return lut::LUT_SIDE_STR[s];
	}

	force_inline const char* CastlingRightsToString(byte castling_rights)
	{
		return lut::LUT_CASTLING_RIGHTS_STR[castling_rights];
	}

} // namespace anka