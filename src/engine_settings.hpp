#pragma once

namespace anka {
	struct EngineSettings {
		static constexpr const char* ENGINE_NAME = "Anka";
		static constexpr const char* AUTHOR_NAME = "Mehmet Eren Yesilyurt";
		static constexpr const char* ENGINE_INFO_STRING = "Anka Chess Engine, Mehmet Eren Yesilyurt";
		static constexpr int DEFAULT_HASH_SIZE = 32;
		static constexpr int MIN_HASH_SIZE = 1;
		static constexpr int MAX_HASH_SIZE = 2000000; // 2 TB
		static constexpr int DEFAULT_NUM_THREADS = 1;
		static constexpr int MIN_NUM_THREADS = 1;
		static constexpr int MAX_NUM_THREADS = 128;
		static constexpr int MAX_SCORE = 32767;
		static constexpr int MIN_SCORE = -32767;
		static constexpr int MAX_DEPTH = 255;
		static constexpr int PV_BUFFER_LENGTH = 64;
		static constexpr int MOVE_OVERHEAD = 10;

		int hash_size = DEFAULT_HASH_SIZE;
		int num_threads = DEFAULT_NUM_THREADS;
		bool null_move_pruning = true;
	};

	inline constexpr int ANKA_INFINITE = EngineSettings::MAX_SCORE;
	inline constexpr int LOWER_MATE_THRESHOLD = -ANKA_INFINITE + EngineSettings::MAX_DEPTH;
	inline constexpr int UPPER_MATE_THRESHOLD = ANKA_INFINITE - EngineSettings::MAX_DEPTH;

	struct SearchParams {
		bool infinite = true;
		bool null_move_pruning = true;

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

		void Clear()
		{
			infinite = true;
			null_move_pruning = true;

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
		}
	};

	
}