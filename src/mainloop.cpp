#include "mainloop.hpp"
#include "search.hpp"
#include "io.hpp"
#include "ttable.hpp"
#include <string.h>
#include <iostream>
#include <future>

	
static constexpr size_t MAX_COMMAND_LENGTH = 4096;
namespace anka {
	TranspositionTable trans_table;
}

void anka::UciLoop()
{
	using namespace uci;
	printf("%s\n", EngineSettings::ENGINE_INFO_STRING);

	EngineSettings options;
	trans_table.Init(options.hash_size);

	SearchParams search_params;
	GameState root_pos;
	root_pos.LoadStartPosition();
		
		
	char* buffer = new char[MAX_COMMAND_LENGTH];

	bool exit_loop = false;
	while (!exit_loop) {
		//io::Read(buffer, MAX_COMMAND_LENGTH);
		fgets(buffer, MAX_COMMAND_LENGTH, stdin);
		char* line = buffer;
		if (strncmp(line, "go", 2) == 0) {
			line += 2;
			OnGo(root_pos, line, search_params);
			if (search_params.uci_quit_flag)
				exit_loop = true;
		}
		else if (strncmp(line, "stop", 4) == 0) {
			search_params.uci_stop_flag = true;
		}
		else if (strncmp(line, "position ", 9) == 0) {
			line += 9;
			OnPosition(root_pos, line);
		}
		else if (strncmp(line, "isready", 7) == 0) {
			OnIsReady();
		}
		else if (strncmp(line, "ucinewgame", 10) == 0) {
			trans_table.Clear();
		}
		else if (strncmp(line, "uci", 3) == 0) {
			OnUci();
		}
		else if (strncmp(line, "setoption ", 10) == 0) {
			line += 10;
			OnSetOption(options, line);
		}
		else if (strncmp(line, "quit", 4) == 0) {
			exit_loop = true;
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

		fflush(stdin);
	}

	delete[] buffer;
}


