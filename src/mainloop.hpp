#pragma once
#include "core.hpp"
#include "gamestate.hpp"
#include "tests/perft.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "evaluation.hpp"
#include "search.hpp"
#include "ttable.hpp"
#include "io.hpp"
#include <future>
#include <string.h>


namespace anka {
	namespace uci {
		inline void OnUci()
		{
			printf("id name Anka\n");
			printf("id author Mehmet Eren Yesilyurt\n");
			printf("option name Hash type spin default %d min %d max %d\n",
				EngineSettings::DEFAULT_HASH_SIZE,
				EngineSettings::MIN_HASH_SIZE,
				EngineSettings::MAX_HASH_SIZE);
			printf("uciok\n");
		}

		inline void OnIsReady()
		{
			printf("readyok\n");
		}

		inline void OnSetOption(EngineSettings &options, char* line)
		{
			// skip next word ("name ")
			line += 5;

			// ex: setoption name Hash value 64
			if (strncmp(line, "Hash value ", 11) == 0) {
				line += 11;
				int size = atoi(line);
				if (size >= EngineSettings::MIN_HASH_SIZE && size <= EngineSettings::MAX_HASH_SIZE) {

					options.hash_size = size;
					if (!trans_table.Init(options.hash_size)) {
						printf("AnkaError(MainLoop): Failed to initialize transposition table\n");
					}
				}
			}
		}

		inline void OnPosition(GameState &pos, char* line)
		{
			// ex: position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
			if (strncmp(line, "startpos", 8) == 0) {
				pos.LoadStartPosition();
				line += 8;
			}
			else if (strncmp(line, "fen", 3) == 0) {
				line += 4;
				if (!pos.LoadPosition(line))
					return;
			}
			else {
				return;
			}

			line = strstr(line, "moves");

			// parse moves
			if (line) {
				line+= 6;
				line = strtok(line, " ");
				while (line) {
					Move move = pos.ParseMove(line);
					if (move == 0) {
						fprintf(stderr, "AnkaError(MainLoop): Failed to parse position input.\n");
						exit(EXIT_FAILURE);
					}

					pos.MakeMove(move);
					line = strtok(NULL, " ");
				}
			}
		}

		inline void OnGo(GameState &root_pos, char* line, SearchParams &params)
		{			
			params.Clear();
			params.infinite = true;

			const char* pch = strstr(line, "infinite");
			if (pch) {
				return IterativeDeepening(root_pos, params);
			}

			pch = strstr(line, "depth ");
			if (pch) {
				params.infinite = false;
				pch += 6;
				int val = atoi(pch);
				if (val > 0)
					params.search_depth = val;
			}

			pch = strstr(line, "movetime ");
			if (pch) {
				params.infinite = false;
				pch += 9;
				int val = atoi(pch);
				if (val > 0)
					params.movetime = val;
				IterativeDeepening(root_pos, params);
				return;
			}

			pch = strstr(line, "movestogo ");
			if (pch) {
				params.infinite = false;
				pch += 10;
				int val = atoi(pch);
				if (val > 0)
					params.movestogo = val;
			}

			pch = strstr(line, "wtime ");
			if (pch) {
				params.infinite = false;
				pch += 6;
				int val = atoi(pch);
				if (val > 0)
					params.wtime = val;
			}

			pch = strstr(line, "btime ");
			if (pch) {
				params.infinite = false;
				pch += 6;
				int val = atoi(pch);
				if (val > 0)
					params.btime = val;
			}

			pch = strstr(line, "winc ");
			if (pch) {
				pch += 5;
				int val = atoi(pch);
				if (val > 0)
					params.winc = val;
			}

			pch = strstr(line, "binc ");
			if (pch) {
				pch += 5;
				int val = atoi(pch);
				if (val > 0)
					params.binc = val;
			}


			IterativeDeepening(root_pos, params);
		}


		inline void OnPrint(GameState &pos)
		{
			pos.Print();
		}

		inline void OnPerft(GameState& pos, char *line)
		{
			int depth = atoi(line);

			if (depth == 0) {
				printf("AnkaError(MainLoop): Invalid perft depth\n");
				return;
			}

			RunPerft(pos, depth);
		}

		inline void OnEval(GameState& pos)
		{
			int eval_score = ClassicalEvaluation(pos);
			printf("Static eval: %+.2f (%+d cp)\n", eval_score / 100.0f, eval_score);

			#ifdef EVAL_DEBUG
			for (int phase = 0; phase < 2; phase++) {
				if (phase == MIDGAME)
					printf("\nMIDGAME EVAL\n");
				else
					printf("\nENDGAME EVAL\n");
				
				printf("            WHITE \tBLACK\n");
				printf("Material:    %d    \t%d\n", eval_info.material[phase][WHITE], eval_info.material[phase][BLACK]);
				printf("Mobility:    %d    \t%d\n", eval_info.mobility[phase][WHITE], eval_info.mobility[phase][BLACK]);
				printf("PST Bonus:   %d    \t%d\n", eval_info.pst[phase][WHITE], eval_info.pst[phase][BLACK]);
				printf("Bshp pair:   %d    \t%d\n", eval_info.bishop_pair[phase][WHITE], eval_info.bishop_pair[phase][BLACK]);
				printf("Pass pawns:  %d    \t%d\n", eval_info.passed[phase][WHITE], eval_info.passed[phase][BLACK]);
				printf("Iso. pawns:  %d    \t%d\n", eval_info.isolated[phase][WHITE], eval_info.isolated[phase][BLACK]);
			}

			printf("Tempo: W: %d B: %d\n", eval_info.tempo[WHITE], eval_info.tempo[BLACK]);
			printf("Phase: %d\n", eval_info.phase);
			#endif // EVAL_DEBUG

		}
	}


	void UciLoop();
}
