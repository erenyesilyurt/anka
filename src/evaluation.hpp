#pragma once
#include "gamestate.hpp"

namespace anka {
	void InitPST();
	int ClassicalEvaluation(const GameState& pos);
}