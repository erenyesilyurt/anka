#include <stdarg.h>
#include <stdio.h>
#include <mutex>
#include "search.hpp"

namespace anka {
	namespace io {
		void PrintToStdout(const char* format, ...);
		void PrintSearchResults(GameState &pos, const SearchResult& result);
		void IndentedPrint(int num_spaces, const char* format, ...);
	} // namespace io
}