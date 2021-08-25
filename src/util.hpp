#pragma once
#include "core.hpp"

namespace anka
{
	template <typename T>
	force_inline T Max(T x, T y)
	{
		return (x > y ? x : y);
	}

	template <typename T>
	force_inline T Min(T x, T y)
	{
		return (x < y ? x : y);
	}

	template <typename T>
	force_inline T Clamp(T val, T low, T high)
	{
		return (val < low) ? low : ((val > high) ? high : val);
	}
}