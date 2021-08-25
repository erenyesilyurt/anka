#pragma once
#include "gamestate.hpp"
#include <math.h>
#include <array>

namespace anka {

	struct EvalScore {
		int phase_score[NUM_PHASES]{};
	};

	struct EvalInfo {
		int material[NUM_PHASES][NUM_SIDES] {};
		int mobility[NUM_PHASES][NUM_SIDES]{};
		int pst[NUM_PHASES][NUM_SIDES]{};
		int pawn_structure[NUM_PHASES][NUM_SIDES]{};
		int bishop_pair[NUM_PHASES][NUM_SIDES]{};
		int phase{};
		int tempo[NUM_SIDES]{};
		int score[NUM_PHASES]{};

		void CalculateScore()
		{
			for (int phase = MIDGAME; phase < NUM_PHASES; phase++) {
				score [phase] = (material[phase][WHITE] - material[phase][BLACK])
					+ (mobility[phase][WHITE] - mobility[phase][BLACK])
					+ (pst[phase][WHITE] - pst[phase][BLACK])
					+ (pawn_structure[phase][WHITE] - pawn_structure[phase][BLACK])
					+ (bishop_pair[phase][WHITE] - bishop_pair[phase][BLACK]);
			}
		}

	};

	extern EvalInfo eval_data;


	struct EvalParams {
		static constexpr int NUM_PARAMS = 787;
		

		int PST_mg[2][8][64]{ 0 };
		int PST_eg[2][8][64]{ 0 };

		int piece_values[NUM_PHASES][8] = {
			{0, 0, 85, 346, 342, 463, 927, 0},
			{0, 0, 95, 302, 305, 516, 927, 0}
		};

		int mobility_weights[NUM_PHASES][8] = {
			{0, 0, 2, 4, 5, 6, 1, 0},
			{0, 0, 2, 0, 2, 3, 9, 0}
		};

		int passed_bonus[NUM_PHASES][8] = {
			{0, 3, 1, 3, 13, 46, 69, 0},
			{0, 2, 3, 21, 40, 59, 73, 0}
		};

		int isolated_penalty[NUM_PHASES][8] = {
			{20, 15, 15, 24, 30, 17, 14, 27},
			{-4, 7, 10, 21, 18, 7, 5, 1}
		};

		int bishop_pair_bonus[NUM_PHASES] = { 34, 43 };
		int tempo_bonus = 18;

		int pawns_pst_mg[64]{
			0, 0, 0, 0, 0, 0, 0, 0,
			68, 81, 68, 77, 72, 70, 54, 63,
			29, 37, 24, 11, 39, 47, 27, 15,
			-5, 11, 3, 25, 19, 16, 5, -18,
			-17, -11, 9, 18, 22, 6, -12, -17,
			-7, -13, 6, -1, 10, 3, 10, 0,
			-13, -7, -12, -9, -2, 23, 16, -4,
			0, 0, 0, 0, 0, 0, 0, 0
		};

		int pawns_pst_eg[64]{
			0, 0, 0, 0, 0, 0, 0, 0,
			81, 72, 74, 82, 77, 72, 75, 72,
			36, 28, 45, 32, 22, 25, 31, 39,
			14, 13, 3, 1, 10, -9, 8, 12,
			4, 10, -12, -3, -10, -13, 0, -4,
			-9, 7, -12, 4, 0, -8, -5, -12,
			-1, 3, 4, 5, -1, -4, -4, -15,
			0, 0, 0, 0, 0, 0, 0, 0
		};

		int knights_pst_mg[64]{
			-74, -58, -46, -44, -12, -58, -69, -71,
			-62, -47, 23, 14, -1, 28, -3, -34,
			-43, 21, 15, 39, 43, 26, 23, 4,
			-22, 4, -4, 33, 13, 37, 8, -6,
			-21, 0, 9, 0, 16, 6, 13, -14,
			-30, -16, 0, 6, 18, 11, 15, -20,
			-29, -47, -13, 1, 5, 12, -23, -17,
			-82, -17, -43, -27, -10, -18, -15, -46
		};

		int knights_pst_eg[64]{
			-75, -49, -17, -34, -23, -49, -60, -75,
			-30, -8, -17, -2, -13, -27, -31, -54,
			-30, -13, 13, 6, -1, 10, -10, -39,
			-14, 5, 27, 23, 25, 16, 5, -11,
			-17, -9, 14, 26, 16, 19, 5, -21,
			-23, -3, -4, 12, 7, -8, -17, -24,
			-40, -22, -9, -10, -8, -18, -19, -51,
			-38, -43, -23, -14, -25, -23, -50, -66
		};

		int bishops_pst_mg[64]{
			-45, -24, -43, -43, -24, -30, -28, -16,
			-30, -11, -36, -23, 8, 23, 7, -28,
			-32, 10, 24, 10, 13, 33, 17, -20,
			-20, -16, -4, 27, 20, 10, -9, -12,
			-16, -3, -8, 17, 11, -15, -7, 1,
			-10, 6, 9, -7, -2, 16, 5, -5,
			4, 13, 5, -5, 0, 12, 22, -3,
			-32, 4, -9, -13, -13, -13, -36, -33
		};

		int bishops_pst_eg[64]{
			-19, -17, -20, -11, -9, -16, -17, -26,
			-11, -5, 3, -16, -11, -8, -10, -30,
			-3, -8, -9, -3, -12, -7, -2, 1,
			-2, 7, 8, 0, 0, 3, -3, -3,
			-7, -3, 9, 5, -1, 5, -5, -9,
			-8, -4, 1, 6, 12, -7, -5, -11,
			-18, -21, -12, -6, 1, -10, -11, -28,
			-24, -13, -17, -3, -6, -11, -6, -12
		};

		int rooks_pst_mg[64]{
			2, 8, -8, 14, 20, -3, 1, 6,
			5, 10, 38, 36, 36, 46, 15, 16,
			-10, 6, 2, 2, -1, 15, 35, 3,
			-20, -30, -5, 11, 3, 16, -18, -26,
			-31, -21, -12, -13, 0, -18, -3, -30,
			-27, -20, -12, -15, 0, -5, -14, -26,
			-32, -9, -9, 0, 12, 10, -10, -15,
			-9, -4, 14, 21, 24, 16, -23, -8
		};

		int rooks_pst_eg[64]{
			19, 13, 19, 13, 13, 16, 13, 12,
			21, 20, 13, 17, 6, 5, 13, 12,
			12, 11, 12, 11, 5, 1, -1, 0,
			13, 15, 16, 2, 7, 7, 6, 13,
			12, 13, 13, 9, 0, 5, 1, 5,
			5, 9, 1, 6, -3, -1, 3, -6,
			2, -2, 4, 8, -3, -3, -3, -12,
			-2, 4, -1, -2, -6, -7, 0, -17
		};

		int queens_pst_mg[64]{
			-28, -2, 2, -1, 15, 17, 6, 2,
			-34, -26, -11, -6, -2, 34, 23, 11,
			-23, -11, -14, -6, 17, 46, 18, 23,
			-27, -29, -20, -24, -1, 2, 1, -6,
			-15, -17, -11, -21, -11, -12, -5, -4,
			-25, -2, -7, -5, -7, -5, 7, 2,
			-27, -9, 7, 4, 15, 15, -7, 10,
			-3, -9, -1, 15, -2, -16, -17, -38
		};

		int queens_pst_eg[64]{
			-12, 11, 11, 10, 33, 16, 13, 5,
			-8, -13, 6, 18, 19, 21, 20, 10,
			-12, -16, -12, 14, 24, 21, 26, 17,
			-5, -6, -13, 6, 15, 19, 33, 16,
			-14, -4, -9, 24, 2, 20, 22, 26,
			7, -22, -6, -13, -1, 17, 11, 16,
			-10, -16, -17, -8, -17, -13, -20, -15,
			-11, -21, -11, -18, -3, -12, -14, -37
		};

		int king_pst_mg[64]{
			-35, -13, -18, -32, -59, -13, -13, -3,
			-5, -22, -14, -24, -35, -13, -21, -19,
			6, -17, -11, -35, -49, -3, -16, -7,
			-4, -22, -20, -54, -62, -36, -15, -17,
			-37, -19, -36, -61, -59, -39, -27, -49,
			-4, -10, -33, -36, -45, -42, -9, -19,
			20, 21, -11, -22, -33, -22, 24, 30,
			5, 52, 24, -10, 12, -16, 47, 27
		};

		int king_pst_eg[64]{
			-74, -31, -22, -25, -16, -1, -13, -32,
			-14, 9, 6, 10, 11, 16, 6, -6,
			-1, 14, 19, 12, 14, 37, 14, -13,
			-20, 17, 19, 27, 28, 28, 16, -11,
			-27, -7, 15, 24, 24, 18, 1, -18,
			-29, -10, 6, 14, 19, 12, 0, -19,
			-44, -25, -3, 0, 6, 1, -18, -38,
			-76, -53, -32, -32, -36, -22, -44, -64
		};

		void UpdateParams(const std::array<int, NUM_PARAMS>& new_params)
		{
			for (Square sq = 0; sq <= square::H8; sq++) {
				knights_pst_mg[sq] = new_params[sq];
				knights_pst_eg[sq] = new_params[64 + sq];
				bishops_pst_mg[sq] = new_params[128 + sq];
				bishops_pst_eg[sq] = new_params[192 + sq];
				rooks_pst_mg[sq] = new_params[256 + sq];
				rooks_pst_eg[sq] = new_params[320 + sq];
				queens_pst_mg[sq] = new_params[384 + sq];
				queens_pst_eg[sq] = new_params[448 + sq];
				king_pst_mg[sq] = new_params[512 + sq];
				king_pst_eg[sq] = new_params[576 + sq];
			}

			for (int i = 0; i < 48; i++) {
				pawns_pst_mg[8 + i] = new_params[640 + i];
				pawns_pst_eg[8 + i] = new_params[688 + i];
			}

			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 5; j++) {
					piece_values[i][2+j] = new_params[736 + 5*i + j];
					mobility_weights[i][2+j] = new_params[746 + 5*i + j];
				}

				for (int j = 0; j < 6; j++) {
					passed_bonus[i][1+j] = new_params[756 + i * 6 + j];
				}

				for (int j = 0; j < 8; j++) {	
					isolated_penalty[i][j] = new_params[768 + i * 8 + j];
				}
			}

			bishop_pair_bonus[0] = new_params[784];
			bishop_pair_bonus[1] = new_params[785];
			tempo_bonus = new_params[786];

			InitPST();
		}

		void Flatten(std::array<int, NUM_PARAMS>& params) const
		{
			for (Square sq = 0; sq <= square::H8; sq++) {
				params[sq] = knights_pst_mg[sq];
				params[64 + sq] = knights_pst_eg[sq];
				params[128 + sq] = bishops_pst_mg[sq];
				params[192 + sq] = bishops_pst_eg[sq];
				params[256 + sq] = rooks_pst_mg[sq];
				params[320 + sq] = rooks_pst_eg[sq];
				params[384 + sq] = queens_pst_mg[sq];
				params[448 + sq] = queens_pst_eg[sq];
				params[512 + sq] = king_pst_mg[sq];
				params[576 + sq] = king_pst_eg[sq];
			}

			for (int i = 0; i < 48; i++) {
				params[640 + i] = pawns_pst_mg[8 + i];
				params[688 + i] = pawns_pst_eg[8 + i];
			}

			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 5; j++) {
					params[736 + 5 * i + j] = piece_values[i][2 + j];
					params[746 + 5 * i + j] = mobility_weights[i][2 + j];
				}

				for (int j = 0; j < 6; j++) {
					params[756 + i * 6 + j] = passed_bonus[i][1 + j];
				}

				for (int j = 0; j < 8; j++) {
					params[768 + i * 8 + j] = isolated_penalty[i][j];
				}
			}

			params[784] = bishop_pair_bonus[0];
			params[785] = bishop_pair_bonus[1];
			params[786] = tempo_bonus;
		}
	

		void InitPST()
		{
			for (Side color = WHITE; color < NUM_SIDES; color++) {
				for (Square sq = square::A1; sq <= square::H8; sq++) {
					Square pst_sq = (color == WHITE ? sq ^ 56 : sq);

					PST_mg[color][piece_type::PAWN][sq] = pawns_pst_mg[pst_sq];
					PST_mg[color][piece_type::KNIGHT][sq] = knights_pst_mg[pst_sq];
					PST_mg[color][piece_type::BISHOP][sq] = bishops_pst_mg[pst_sq];
					PST_mg[color][piece_type::ROOK][sq] = rooks_pst_mg[pst_sq];
					PST_mg[color][piece_type::QUEEN][sq] = queens_pst_mg[pst_sq];
					PST_mg[color][piece_type::KING][sq] = king_pst_mg[pst_sq];

					PST_eg[color][piece_type::PAWN][sq] = pawns_pst_eg[pst_sq];
					PST_eg[color][piece_type::KNIGHT][sq] = knights_pst_eg[pst_sq];
					PST_eg[color][piece_type::BISHOP][sq] = bishops_pst_eg[pst_sq];
					PST_eg[color][piece_type::ROOK][sq] = rooks_pst_eg[pst_sq];
					PST_eg[color][piece_type::QUEEN][sq] = queens_pst_eg[pst_sq];
					PST_eg[color][piece_type::KING][sq] = king_pst_eg[pst_sq];
				}
			}
		}

		#define WRITE_PST(pst_name, file_stream) \
			do { fprintf(file_stream, "int " STRINGIFY(pst_name) "[64]{"); \
			for (Square sq = square::A1; sq < square::H8; sq++) { \
					if (sq % 8 == 0) fprintf(file_stream, "\n\t"); \
					fprintf(file_stream, "%d, ", pst_name[sq]); \
			} \
			fprintf(file_stream, "%d\n};\n\n", pst_name[square::H8]); } while (0)


		#define WRITE_FEATURE(feature, file_stream) \
			do {int* p = feature[MIDGAME]; \
			fprintf(file_stream, "int " STRINGIFY(feature) "[NUM_PHASES][8] = {\n\t"); \
			fprintf(file_stream, "{%d, %d, %d, %d, %d, %d, %d, %d},\n\t", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]); \
			p = feature[ENDGAME]; \
			fprintf(file_stream, "{%d, %d, %d, %d, %d, %d, %d, %d}\n};\n\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]); } while (0)

		void WriteToFile(const char *file_name)
		{
			FILE* file_stream = fopen(file_name, "w");
			if (file_stream == NULL) {
				fprintf(stderr, "Failed to open %s\n", file_name);
				return;
			}

			fprintf(file_stream, "// ***Generated by the evaluation tuner***\n");
			WRITE_FEATURE(piece_values, file_stream);
			WRITE_FEATURE(mobility_weights, file_stream);
			WRITE_FEATURE(passed_bonus, file_stream);
			WRITE_FEATURE(isolated_penalty, file_stream);

			fprintf(file_stream, "int bishop_pair_bonus[NUM_PHASES] = { %d, %d };\n", bishop_pair_bonus[0], bishop_pair_bonus[1]);

			fprintf(file_stream, "int tempo_bonus = %d;\n\n", tempo_bonus);

			WRITE_PST(pawns_pst_mg, file_stream);
			WRITE_PST(pawns_pst_eg, file_stream);

			WRITE_PST(knights_pst_mg, file_stream);
			WRITE_PST(knights_pst_eg, file_stream);

			WRITE_PST(bishops_pst_mg, file_stream);
			WRITE_PST(bishops_pst_eg, file_stream);

			WRITE_PST(rooks_pst_mg, file_stream);
			WRITE_PST(rooks_pst_eg, file_stream);

			WRITE_PST(queens_pst_mg, file_stream);
			WRITE_PST(queens_pst_eg, file_stream);

			WRITE_PST(king_pst_mg, file_stream);
			WRITE_PST(king_pst_eg, file_stream);
			

			fclose(file_stream);
			#undef WRITE_FEATURE
			#undef WRITE_PST		
		}
	};
	
	extern EvalParams eval_params;
}