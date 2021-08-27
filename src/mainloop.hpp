#pragma once
#include "core.hpp"
#include "gamestate.hpp"
#include "tests/perft.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "evaluation.hpp"
#include "search.hpp"
#include "ttable.hpp"
#include "io.hpp"
#include <future>
#include <string.h>


namespace anka {
	namespace uci {
		void UciLoop();
		void OnUci();
		void OnIsReady();
		void OnSetOption(EngineSettings& options, char* line);
		void OnPosition(GameState& pos, char* line);
		void OnGo(const GameState& root_pos, char* line, SearchParams& params);
		void OnPrint(GameState& pos);
		void OnPerft(GameState& pos, char* line);
		void OnEval(GameState& pos);
	}


	
}
