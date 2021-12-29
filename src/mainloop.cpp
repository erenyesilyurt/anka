#include "mainloop.hpp"
#include "search.hpp"
#include "ttable.hpp"
#include "tbprobe.h"
#include <string.h>
#include <iostream>
#include <future>


namespace {
	constexpr size_t MAX_COMMAND_LENGTH = 4096;
	long long AllocateTime(const anka::GameState& pos, int wtime, int winc, int btime, int binc, int movestogo)
	{
		using namespace anka;
		long long available_time = 0;
		long long increment = 0;
		if (pos.SideToPlay() == WHITE) {
			available_time = wtime;
			increment = winc;
		}
		else {
			available_time = btime;
			increment = binc;
		}

		if (movestogo == 1) {
			return ((available_time * 3) / 4) - EngineSettings::MOVE_OVERHEAD;
		}
		else if (movestogo <= 0) {
			movestogo = 30;
		}

		available_time += (movestogo / 2) * increment;
		available_time = available_time / movestogo;	
		available_time -= EngineSettings::MOVE_OVERHEAD;

		return available_time;
	}
}
namespace anka {
	void uci::UciLoop()
	{
		using namespace uci;
		printf("%s\n", EngineSettings::ENGINE_INFO_STRING);

		EngineSettings options;	
		SearchParams search_params;
		GameState root_pos;
		root_pos.LoadStartPosition();


		char* buffer = new char[MAX_COMMAND_LENGTH];

		std::future<void> future;
		while (true) {
			fgets(buffer, MAX_COMMAND_LENGTH, stdin);
			char* line = buffer;

			if (strncmp(line, "stop", 4) == 0) {
				search_params.uci_stop_flag = true;
				if (future.valid())
					future.get();
			}
			else if (strncmp(line, "quit", 4) == 0) {
				search_params.uci_stop_flag = true;
				if (future.valid())
					future.get();
				break;
			}

			if (search_params.is_searching) {
				if (strncmp(line, "isready", 7) == 0) {
					if (search_params.uci_stop_flag) {
						future.get(); // wrap up the last search
					}
					OnIsReady();
				}
				else {
					fprintf(stderr, "Received command while searching: %s\n", line);
				}
			}
			else {
				if (strncmp(line, "isready", 7) == 0) {
					OnIsReady();
				}
				else if (strncmp(line, "go", 2) == 0) {
					line += 2;
					OnGo(root_pos, line, search_params);
					search_params.is_searching = true;
					future = std::async(std::launch::async, StartSearch, std::ref(root_pos), std::ref(search_params));
				}
				else if (strncmp(line, "position ", 9) == 0) {
					line += 9;
					OnPosition(root_pos, line);
				}
				else if (strncmp(line, "ucinewgame", 10) == 0) {
					g_trans_table.Clear();
				}
				else if (strncmp(line, "uci", 3) == 0) {
					OnUci();
				}
				else if (strncmp(line, "setoption ", 10) == 0) {
					line += 10;
					OnSetOption(options, line);
				}
				else if (strncmp(line, "anka_print", 10) == 0) {
					OnPrint(root_pos);
				}
				else if (strncmp(line, "anka_perft ", 11) == 0) {
					line += 11;
					OnPerft(root_pos, line);
				}
				else if (strncmp(line, "anka_eval", 9) == 0) {
					OnEval(root_pos);
				}
			}
		}

		delete[] buffer;
	}

	void uci::OnUci()
	{
		printf("id name Anka\n");
		printf("id author Mehmet Eren Yesilyurt\n");
		printf("option name Hash type spin default %d min %d max %d\n",
			EngineSettings::DEFAULT_HASH_SIZE,
			EngineSettings::MIN_HASH_SIZE,
			EngineSettings::MAX_HASH_SIZE);
		printf("option name SyzygyPath type string default null\n");
		printf("uciok\n");
	}

	void uci::OnIsReady()
	{
		printf("readyok\n");
	}

	void uci::OnSetOption(EngineSettings& options, char* line)
	{
		// skip the next word ("name ")
		line += 5;

		// setoption name Hash value 64
		if (strncmp(line, "Hash value ", 11) == 0) {
			line += 11;
			int size = atoi(line);
			if (size >= EngineSettings::MIN_HASH_SIZE && size <= EngineSettings::MAX_HASH_SIZE) {

				options.hash_size = size;
				if (!g_trans_table.Init(options.hash_size)) {
					fprintf(stderr, "AnkaError(MainLoop): Failed to initialize transposition table\n");
				}
			}
		}

		// setoption name SyzygyPath value /tb
		if (strncmp(line, "SyzygyPath value ", 17) == 0) {
			line += 17;
			line[strcspn(line, "\n")] = '\0';
			if (!tb_init(line) || TB_LARGEST <= 0) {
				fprintf(stderr, "Failed to initialize tablebase or could not find ay TB files\n");
			}
		}

	}

	void uci::OnPosition(GameState& pos, char* line)
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
			line += 6;
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
			pos.SetRootPlyIndex();
		}
	}

	void uci::OnGo(const GameState& root_pos, char* line, SearchParams& params)
	{
		params.Clear();
		params.start_time = Timer::GetTimeInMs();
		int movestogo = 0;
		int wtime = 0;
		int btime = 0;
		int winc = 0;
		int binc = 0;

		const char* pch = strstr(line, "infinite");
		if (pch) {
			params.infinite = true;
			return;
		}

		pch = strstr(line, "depth ");
		if (pch) {
			pch += 6;
			int val = atoi(pch);
			if (val > 0)
				params.depth_limit = val;
			return;
		}

		params.check_timeup = true;
		pch = strstr(line, "movetime ");
		if (pch) {
			pch += 9;
			int val = atoi(pch);
			if (val > 0)
				params.remaining_time = val;
			return;
		}

		pch = strstr(line, "movestogo ");
		if (pch) {
			pch += 10;
			int val = atoi(pch);
			if (val > 0)
				movestogo = val;
		}

		pch = strstr(line, "wtime ");
		if (pch) {
			pch += 6;
			int val = atoi(pch);
			if (val > 0)
				wtime = val;
		}

		pch = strstr(line, "btime ");
		if (pch) {
			pch += 6;
			int val = atoi(pch);
			if (val > 0)
				btime = val;
		}

		pch = strstr(line, "winc ");
		if (pch) {
			pch += 5;
			int val = atoi(pch);
			if (val > 0)
				winc = val;
		}

		pch = strstr(line, "binc ");
		if (pch) {
			pch += 5;
			int val = atoi(pch);
			if (val > 0)
				binc = val;
		}

		params.remaining_time = AllocateTime(root_pos, wtime, winc, btime, binc, movestogo);
	}


	void uci::OnPrint(GameState& pos)
	{
		pos.Print();
	}

	void uci::OnPerft(GameState& pos, char* line)
	{
		int depth = atoi(line);

		if (depth == 0) {
			printf("AnkaError(MainLoop): Invalid perft depth\n");
			return;
		}

		RunPerft(pos, depth);
	}

	void uci::OnEval(GameState& pos)
	{
		int eval_score = pos.ClassicalEvaluation();
		printf("Static eval: %+.2f (%+d cp)\n", eval_score / 100.0f, eval_score);
	}

}


