#include "io.hpp"
#include "search.hpp"
#include "engine_settings.hpp"
#include <math.h>
#include <inttypes.h>



bool anka::io::InputIsAvailable()
{
	auto stdin_fd = fileno(stdin);

	#ifdef PLATFORM_UNIX
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(stdin_fd, &readfds);
	timeval tv{};
	select(16, &readfds, 0, 0, &tv);

	return (FD_ISSET(stdin_fd, &readfds));

	#else // WINDOWS
	static HANDLE hIn;
	static bool initialized = false, is_pipe = false;
	if (!initialized) {
		initialized = true;
		hIn = GetStdHandle(STD_INPUT_HANDLE);
		DWORD console_mode;
		is_pipe = !GetConsoleMode(hIn, &console_mode);

		if (!is_pipe) {
			SetConsoleMode(hIn, console_mode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(hIn);
		}	
	}
	
	if (is_pipe) {
		DWORD n_bytes = 0;
		if (PeekNamedPipe(hIn, NULL, 0, NULL, &n_bytes, NULL)) {
			return (n_bytes > 0);
		}
		else {
			return false;
		}
	}
	else {
		DWORD n_events;
		if (GetNumberOfConsoleInputEvents(hIn, &n_events)) {
			return (n_events > 1);
		}
		else {
			return false;
		}

	}
	#endif

	return false;

}

int anka::io::PolledRead(char* buffer, int buffer_size)
{
	if (!InputIsAvailable())
		return 0;

	int bytes_read = read(fileno(stdin), buffer, buffer_size);
	char* end = strchr(buffer, '\n');
	if (end)
		*end = '\0';
	return bytes_read;
}

int anka::io::Read(char* buffer, int buffer_size)
{
	int bytes_read = read(fileno(stdin), buffer, buffer_size);
	char* end = strchr(buffer, '\n');
	if (end)
		*end = '\0';
	return bytes_read;
}


void anka::io::PrintSearchResults(GameState &pos, const SearchResult& result)
{
	char move_str[6];

	printf("info depth %d time %lld nodes %" PRIu64 " nps %" PRIu64 " score ",
		result.depth, result.total_time, result.total_nodes,
		result.nps);
		
	if (result.best_score >= UPPER_MATE_THRESHOLD) {
		int moves_to_mate = (ANKA_INFINITE - result.best_score);
		printf("mate %d pv ", moves_to_mate);
	}
	else if (result.best_score <= LOWER_MATE_THRESHOLD) {
		int moves_to_mate = (-ANKA_INFINITE - result.best_score);
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
}
	
//// used for debugging
//void io::IndentedPrint(int num_spaces, const char* format, ...)
//{
//	while (num_spaces) {
//		printf("  ");
//		num_spaces--;
//	}

//	va_list args;
//	va_start(args, format);
//	vprintf(format, args);
//	va_end(args);
//}
