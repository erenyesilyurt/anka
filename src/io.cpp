#include "io.hpp"
#include "search.hpp"
#include "engine_settings.hpp"
#include <math.h>
#include <inttypes.h>


static std::mutex stdout_mutex;

namespace anka {

	void io::PrintToStdout(const char* format, ...)
	{
		std::lock_guard<std::mutex> guard(stdout_mutex);
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);

		fflush(stdout);
	}

	void io::PrintSearchResults(GameState &pos, const SearchResult& result)
	{
		// acquire lock
		std::lock_guard<std::mutex> guard(stdout_mutex);

		char move_str[6];
		//printf("info depth %d score cp %d time %lld nodes %llu nps %llu pv ",
		//	result.depth, result.best_score, result.total_time, result.total_nodes,
		//	result.nps);

		printf("info depth %d time %lld nodes %" PRIu64 " nps %" PRIu64 " score ",
			result.depth, result.total_time, result.total_nodes,
			result.nps);
		
		if (result.best_score >= UPPER_MATE_THRESHOLD) {
			//int moves_to_mate = std::ceilf((INFINITE - result.best_score) / 2.0f);
			int moves_to_mate = (INFINITE - result.best_score);
			printf("mate %d pv ", moves_to_mate);
		}
		else if (result.best_score <= LOWER_MATE_THRESHOLD) {
			//int moves_to_mate = std::floorf((-INFINITE - result.best_score) / 2.0f);
			int moves_to_mate = (-INFINITE - result.best_score);
			printf("mate %d pv ", moves_to_mate);
		}
		else {
			printf("cp %d pv ", result.best_score);
		}

		for (int i = 0; i < result.pv_length; i++) {
			move::ToString(result.pv[i], move_str);
			printf("%s ", move_str);
		}	

		putchar('\n');
		//#ifdef ANKA_DEBUG
		//printf("Order Quality: %.2f\n", result.fh_f / (float)result.fh);
		//#endif // ANKA_DEBUG

		fflush(stdout);
	}
	
	// used for debugging
	void io::IndentedPrint(int num_spaces, const char* format, ...)
	{
		while (num_spaces) {
			printf("  ");
			num_spaces--;
		}

		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}