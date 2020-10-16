#include "perft.hpp"
#include "movegen.hpp"
#include "timer.hpp"


namespace anka {

	u64 perft(GameState& pos, int depth)
	{
		MoveList<256> list;
		u64 num_nodes = 0;

		list.GenerateLegalMoves(pos);

		if (depth == 1)
			return list.length;

		for (int i = 0; i < list.length; i++) {
			pos.MakeMove(list.moves[i].move);
			num_nodes += perft(pos, depth - 1);
			pos.UndoMove();
		}

		return num_nodes;
	}

	u64 RunPerft(GameState &pos, int depth)
	{
		if (depth == 0)
			return 1;

		
		MoveList<256> list;
		u64 num_nodes = 0;

		char move_str[6];
		auto start_time = Timer::GetTimeInMs();
		list.GenerateLegalMoves(pos);
		if (depth == 1) {
			for (int i = 0; i < list.length; i++) {
				move::ToString(list.moves[i].move, move_str);
				fprintf(stdout, "%s: %llu\n", move_str, (long long)1);
			}
			num_nodes += list.length;
		}
		else {
			for (int i = 0; i < list.length; i++) {
				pos.MakeMove(list.moves[i].move);
				u64 num_branch_nodes = perft(pos, depth - 1);
				move::ToString(list.moves[i].move, move_str);
				fprintf(stdout, "%s: %llu\n", move_str, num_branch_nodes);
				num_nodes += num_branch_nodes;
				pos.UndoMove();
			}
		}
		auto end_time = Timer::GetTimeInMs();

		auto time_in_ms = end_time - start_time;
		auto time_in_seconds = time_in_ms / 1000.0f;
		u64 nodes_per_sec = num_nodes / time_in_seconds;
		std::cout << "\nNodes searched: " << num_nodes << "\n";
		std::cout << "Time(s): " << time_in_seconds << "\n";
		std::cout << "Nodes/second: " << nodes_per_sec << "\n" << std::endl;
		//fprintf(stdout, "\nNodes searched: %llu\n", num_nodes);
		return num_nodes;
	}
}