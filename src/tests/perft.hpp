#pragma once
#include "core.hpp"
#include "gamestate.hpp"

namespace anka {
	u64 perft(GameState& pos, int depth);
	u64 RunPerft(GameState& pos, int depth);
}