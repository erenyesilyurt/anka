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

	extern TransposTable trans_table;

	namespace uci {
		//enum class CommandType {
		//	invalid_cmd, debug, uci, isready, setoption, ucinewgame, position, go, stop, ponderhit, quit, registr,
		//	anka_print, anka_perft, anka_eval
		//};

		//struct UciCommand {
		//	CommandType type = CommandType::invalid_cmd;
		//	char* value = nullptr;
		//};


		inline void OnUci()
		{
			printf("id name Anka\n");
			printf("id author Mehmet Eren Yesilyurt\n");
			printf("option name Hash type spin default %d min %d max %d\n",
				EngineSettings::DEFAULT_HASH_SIZE,
				EngineSettings::MIN_HASH_SIZE,
				EngineSettings::MAX_HASH_SIZE);

			printf("option name Threads type spin default %d min %d max %d\n",
				EngineSettings::DEFAULT_NUM_THREADS,
				EngineSettings::MIN_NUM_THREADS,
				EngineSettings::MAX_NUM_THREADS);

			printf("option name Nullmove type check default true\n");
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
			else if (strncmp(line, "Threads value ", 14) == 0) {
				line += 14;
				int num_threads = atoi(line);
				if (num_threads >= EngineSettings::MIN_NUM_THREADS && num_threads <= EngineSettings::MAX_NUM_THREADS) {
					options.num_threads = num_threads;
				}
			}
			else if (strncmp(line, "Nullmove value ", 15) == 0)  {
				line += 15;
				if (strncmp(line, "true", 4) == 0) {
					options.null_move_pruning = true;
				}
				else if (strncmp(line, "false", 5) == 0) {
					options.null_move_pruning = false;
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
						pos.LoadStartPosition();
						return;
					}

					pos.MakeMove(move);
					line = strtok(NULL, " ");
				}
			}

			pos.SetSearchDepth(0);
		}

		inline void OnGo(GameState &root_pos, char* line, SearchParams &params)
		{			
			params.Clear();
			params.infinite = true;

			const char* pch = strstr(line, "infinite");
			if (pch) {
				IterativeDeepening(root_pos, params);
				return;
			}
			else {
				params.infinite = false;
			}

			pch = strstr(line, "depth ");
			if (pch) {
				pch += 6;
				int val = atoi(pch);
				if (val > 0)
					params.search_depth = val;
			}

			pch = strstr(line, "movetime ");
			if (pch) {
				pch += 9;
				int val = atoi(pch);
				if (val > 0)
					params.movetime = val;
				IterativeDeepening(root_pos, params);
				return;
			}

			pch = strstr(line, "movestogo ");
			if (pch) {
				pch += 10;
				int val = atoi(pch);
				if (val > 0)
					params.movestogo = val;
			}

			pch = strstr(line, "wtime ");
			if (pch) {
				pch += 6;
				int val = atoi(pch);
				if (val > 0)
					params.wtime = val;
			}

			pch = strstr(line, "winc ");
			if (pch) {
				pch += 5;
				int val = atoi(pch);
				if (val > 0)
					params.winc = val;
			}

			pch = strstr(line, "btime ");
			if (pch) {
				pch += 6;
				int val = atoi(pch);
				if (val > 0)
					params.btime = val;
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
			int eval_score = evaluate_basic(pos);
			printf("Static eval: %+.2f (%+d cp)\n", eval_score / 100.0f, eval_score);
		}
	}


	void UciLoop();
}
