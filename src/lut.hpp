#include "core.hpp"

/* Look-up tables (LUT)*/
namespace anka {
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
} // namespace anka