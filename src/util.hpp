#pragma once
#include "core.hpp"

namespace anka
{
	force_inline int Max(int x, int y)
	{
		return (x > y ? x : y);
	}

	force_inline int Min(int x, int y)
	{
		return (x < y ? x : y);
	}

	force_inline int Clamp(int val, int low, int high)
	{
		return (val < low) ? low : ((val > high) ? high : val);
	}
}