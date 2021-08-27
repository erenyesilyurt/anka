#include "io.hpp"
#include "search.hpp"
#include "engine_settings.hpp"



void anka::io::PrintSearchResults(GameState &pos, const SearchResult& result)
{
	char move_str[6];

	printf("info depth %d time %lld nodes %" PRIu64 " nps %" PRIu64 " score ",
		result.depth, result.total_time, result.total_nodes,
		result.nps);
		
	if (result.best_score >= UPPER_MATE_THRESHOLD) {
		int moves_to_mate = ((ANKA_MATE - result.best_score) >> 1) + 1;
		printf("mate %d pv ", moves_to_mate);
	}
	else if (result.best_score <= LOWER_MATE_THRESHOLD) {
		int moves_to_mate = ((-ANKA_MATE - result.best_score) >> 1);
		printf("mate %d pv ", moves_to_mate);
	}
	else {
		printf("cp %d pv ", result.best_score);
	}

	for (int i = 0; i < MAX_PLY; i++) {
		if (result.pv[i] == 0)
			break;
		move::ToString(result.pv[i], move_str);
		printf("%s ", move_str);
	}

	putchar('\n');
	STATS(printf("Order Quality: %.2f\n", result.fh_f / (float)result.fh));
}
