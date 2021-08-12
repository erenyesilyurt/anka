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

	struct SearchParams {
		bool infinite = true;

		long long start_time = 0;
		long long wtime = 0;
		long long btime = 0;
		long long winc = 0;
		long long binc = 0;
		long long movetime = 0;
		int movestogo = 0;
		int search_depth = 0;

		bool check_timeup = false;
		bool uci_stop_flag = false;
		bool uci_quit_flag = false;

		int pv_index = 0;

		void Clear()
		{
			infinite = true;

			start_time = 0;
			wtime = 0;
			btime = 0;
			winc = 0;
			binc = 0;
			movetime = 0;
			movestogo = 0;
			search_depth = 0;

			uci_stop_flag = false;
			uci_quit_flag = false;
			check_timeup = false;

			pv_index = 0;
		}
	};

	
}