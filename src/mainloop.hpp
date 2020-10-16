#pragma once
#include "core.hpp"
#include "gamestate.hpp"
#include "tests/perft.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "evaluation.hpp"
#include "search.hpp"
#include "ttable.hpp"
#include <future>
#include "io.hpp"

namespace anka {

	extern TransposTable trans_table;

	namespace uci {
		enum class CommandType {
			invalid_cmd, debug, uci, isready, setoption, ucinewgame, position, go, stop, ponderhit, quit, registr,
			anka_print, anka_perft, anka_eval
		};

		struct UciCommand {
			CommandType type = CommandType::invalid_cmd;
			char* value = nullptr;
		};

		//UciCommand ParseCommandType(char *line);



		inline Move ParseMove(const GameState& pos, const char* line)
		{
			auto move_str_len = strlen(line);
			if (move_str_len < 4 || move_str_len > 5)
				return 0;

			char move_str[6];
			strcpy(move_str, line);
			

			if (move_str[0] > 'h' || move_str[0] < 'a')
				return 0;

			if (move_str[1] > '8' || move_str[1] < '1')
				return 0;

			if (move_str[2] > 'h' || move_str[2] < 'a')
				return 0;

			if (move_str[3] > '8' || move_str[3] < '1')
				return 0;

			Square from = square::RankFileToSquare(move_str[1] - '1', move_str[0] - 'a');
			Square to = square::RankFileToSquare(move_str[3] - '1', move_str[2] - 'a');

			ANKA_ASSERT(from >= 0 && from < 64);
			ANKA_ASSERT(to >= 0 && to < 64);

			MoveList<256> list;
			list.GenerateLegalMoves(pos);

			// find the matching move from possible moves and return it
			for (int i = 0; i < list.length; i++) {
				Move move = list.moves[i].move;
				if (move::FromSquare(move) == from && move::ToSquare(move) == to) {
					if (move::IsPromotion(move)) {
						if (move_str_len != 5)
							return 0;

						PieceType promoted = move::PromotedPiece(move);
						if (promoted == piece_type::ROOK && move_str[4] == 'r') {
							return move;
						}
						else if (promoted == piece_type::BISHOP && move_str[4] == 'b') {
							return move;
						}
						else if (promoted == piece_type::QUEEN && move_str[4] == 'q') {
							return move;
						}
						else if (promoted == piece_type::KNIGHT && move_str[4] == 'n') {
							return move;
						}
						else {
							return 0;
						}
					}
					return move;
				}
			}

			return 0;
		}

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
			io::PrintToStdout("readyok\n");
		}

		inline void OnSetOption(EngineSettings &options, char* line)
		{
			// skip next word (name)
			line = strtok(NULL, " ");

			if (strcmp(line, "name") != 0)
				return;

			char *option_name = strtok(NULL, " ");

			if (strcmp(option_name, "Hash") == 0) {
				option_name = strtok(NULL, " ");
				char* val = strtok(NULL, " ");
				int size = atoi(val);
				if (size >= EngineSettings::MIN_HASH_SIZE && size <= EngineSettings::MAX_HASH_SIZE) {

					options.hash_size = size;
					if (!trans_table.Init(options.hash_size)) {
						printf("AnkaError(MainLoop): Failed to initialize transposition table\n");
					}
				}
			}
			else if (strcmp(option_name, "Threads") == 0) {
				option_name = strtok(NULL, " ");
				char* val = strtok(NULL, " ");
				int num_threads = atoi(val);
				if (num_threads >= EngineSettings::MIN_NUM_THREADS && num_threads <= EngineSettings::MAX_NUM_THREADS) {
					options.num_threads = num_threads;
				}
			}
			else if (strcmp(option_name, "Nullmove") == 0) {
				option_name = strtok(NULL, " ");
				char* val = strtok(NULL, " ");
				if (strcmp(val, "true") == 0) {
					options.null_move_pruning = true;
				}
				else if (strcmp(val, "false") == 0) {
					options.null_move_pruning = false;
				}
			}
		}


		inline void OnPosition(GameState &pos, char* line)
		{
			line = strtok(NULL, " ");
			// * position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
			if (strcmp(line, "startpos") == 0) {
				pos.LoadStartPosition();
				line += 9;
			}
			else if (strcmp(line, "fen") == 0) {
				line += 4;
				if (!pos.LoadPosition(line))
					return;
			}
			else {
				return;
			}

			char* moves_loc = strstr(line, "moves");

			// parse moves
			if (moves_loc) {
				moves_loc += 6;
				line = strtok(moves_loc, " ");
				while (line) {
					Move move = ParseMove(pos, line);
					if (move == 0) {
						io::PrintToStdout("Failed to parse position input.\n");
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

			// TODO: support restricted search moves
			// TODO: support ponder
			// TODO: support mate search
			// TODO: support movestogo
			// TODO: support node limit
			
			params.Clear();
			params.infinite = false;
			params.is_searching_flag = true;
			while (true) {
				line = strtok(NULL, " ");
				if (!line)
					break;
				if (strcmp(line, "infinite") == 0) {
					params.infinite = true;
					std::thread t(IterativeDeepening, std::ref(root_pos), std::ref(params));
					t.detach();
					return;
				}
				else if (strcmp(line, "wtime") == 0) {
					line = strtok(NULL, " ");
					int val = atoi(line);
					if (val > 0)
						params.wtime = val;
				}
				else if (strcmp(line, "btime") == 0) {
					line = strtok(NULL, " ");
					int val = atoi(line);
					if (val > 0)
						params.btime = val;
				}
				else if (strcmp(line, "winc") == 0) {
					line = strtok(NULL, " ");
					int val = atoi(line);
					if (val > 0)
						params.winc = val;
				}
				else if (strcmp(line, "binc") == 0) {
					line = strtok(NULL, " ");
					int val = atoi(line);
					if (val > 0)
						params.binc = val;
				}
				else if (strcmp(line, "depth") == 0) {
					line = strtok(NULL, " ");
					int val = atoi(line);
					if (val > 0)
						params.search_depth = val;
				}
				else if (strcmp(line, "movetime") == 0) {
					line = strtok(NULL, " ");
					int val = atoi(line);
					if (val > 0)
						params.movetime = val;
				}
			}

			std::thread t(IterativeDeepening, std::ref(root_pos), std::ref(params));
			t.detach();

		}

		inline void OnQuit()
		{

		}

		inline void OnPrint(GameState &pos)
		{
			pos.Print();
		}

		inline void OnPerft(GameState& pos, char *line)
		{
			line = strtok(NULL, " ");
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
