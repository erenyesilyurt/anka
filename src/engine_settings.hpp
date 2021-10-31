#pragma once
#include <limits.h>

namespace anka {
	struct EngineSettings {
		static constexpr const char* ENGINE_NAME = "Anka";
		static constexpr const char* AUTHOR_NAME = "Mehmet Eren Yesilyurt";
		static constexpr const char* ENGINE_INFO_STRING = "Anka Chess Engine, Mehmet Eren Yesilyurt";
		static constexpr int DEFAULT_HASH_SIZE = 32;
		static constexpr int MIN_HASH_SIZE = 1;
		static constexpr int MAX_HASH_SIZE = 1ULL << 18; // 256 GiB
		static constexpr int PV_BUFFER_LENGTH = 64;
		static constexpr int MOVE_OVERHEAD = 10;

		int hash_size = DEFAULT_HASH_SIZE;
	};

	inline constexpr int ANKA_INFINITE = SHRT_MAX;
	inline constexpr int ANKA_MATE = SHRT_MAX / 2;
	inline constexpr int LOWER_MATE_THRESHOLD = -ANKA_MATE + 383;
	inline constexpr int UPPER_MATE_THRESHOLD = ANKA_MATE - 383;
	inline constexpr int MAX_PLY = 128;
}