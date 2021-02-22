#include "search.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <mutex>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <io.h>
#elif defined(PLATFORM_UNIX)
#include <unistd.h>
#include <sys/select.h>
#endif


namespace anka {
	namespace io {
		int PolledRead(char* buffer, int buffer_size);
		int Read(char* buffer, int buffer_size);
		bool InputIsAvailable();
		void PrintSearchResults(GameState &pos, const SearchResult& result);
		void IndentedPrint(int num_spaces, const char* format, ...);
	} // namespace io
}