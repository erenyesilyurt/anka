#pragma once
#include <chrono>
#include <iostream>
#include "core.hpp"

namespace anka {
	class Timer
	{
	public:
		Timer()
		{
			m_startpoint = std::chrono::high_resolution_clock::now();
		}

		force_inline static long long GetTimeInMs()
		{
			auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
			auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now);

			return now_ms.count();
		}

		~Timer()
		{
			auto endpoint = std::chrono::high_resolution_clock::now();
			auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(m_startpoint);
			auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(endpoint);

			auto duration = end - start;

			std::cout << "Time taken: " << duration.count() << "ms" << std::endl;
		}
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_startpoint;
	};
} // namespace anka