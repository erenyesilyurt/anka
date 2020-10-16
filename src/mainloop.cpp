#include "mainloop.hpp"
#include "search.hpp"
#include <string.h>
#include <iostream>
#include "ttable.hpp"
#include <future>
#include "io.hpp"


namespace anka {
	
	static constexpr size_t MAX_COMMAND_LENGTH = 4096;

	TransposTable trans_table;

	
	void UciLoop()
	{
		using namespace uci;
		//printf("%s\n", EngineSettings::ENGINE_INFO_STRING);
		io::PrintToStdout("%s\n", EngineSettings::ENGINE_INFO_STRING);
		fflush(stdout);

		EngineSettings options;
		trans_table.Init(options.hash_size);

		SearchParams search_params;
		GameState root_pos;
		root_pos.LoadStartPosition();
		
		
		char* line = new char[4096];
		bool exit_loop = false;
		while (!exit_loop) {
			std::cin.getline(line, MAX_COMMAND_LENGTH);

			line = strtok(line, " ");

			if (strcmp(line, "go") == 0) {
				if (search_params.is_searching_flag)
					continue;
				
				OnGo(root_pos, line, search_params);
			}
			else if (strcmp(line, "stop") == 0) {
				search_params.uci_stop_flag = true;
			}
			else if (strcmp(line, "position") == 0) {
				if (search_params.is_searching_flag)
					continue;
				OnPosition(root_pos, line);
			}
			else if (strcmp(line, "isready") == 0) {
				OnIsReady();
			}
			else if (strcmp(line, "ucinewgame") == 0) {
				
			}
			else if (strcmp(line, "uci") == 0) {
				if (search_params.is_searching_flag)
					continue;
				OnUci();
			}
			else if (strcmp(line, "setoption") == 0) {
				if (search_params.is_searching_flag)
					continue;
				OnSetOption(options, line);
			}
			else if (strcmp(line, "ponderhit") == 0) {
				
			}
			else if (strcmp(line, "debug") == 0) {
				
			}
			else if (strcmp(line, "register") == 0) {
				
			}
			else if (strcmp(line, "quit") == 0) {
				OnQuit();
				exit_loop = true;
			}
			else if (strcmp(line, "anka_print") == 0) {
				if (search_params.is_searching_flag)
					continue;
				OnPrint(root_pos);
			}
			else if (strcmp(line, "anka_perft") == 0) {
				if (search_params.is_searching_flag)
					continue;
				OnPerft(root_pos, line);
			}
			else if (strcmp(line, "anka_eval") == 0) {
				if (search_params.is_searching_flag)
					continue;
				OnEval(root_pos);
			}

			
			fflush(stdout);
		}

		delete[] line;
	}
}


