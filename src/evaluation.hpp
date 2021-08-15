#pragma once
#include "gamestate.hpp"

namespace anka {

	#ifdef EVAL_DEBUG

	struct EvalInfo {
		int material[NUM_PHASES][2] {};
		int mobility[NUM_PHASES][2]{};
		int pst[NUM_PHASES][2]{};
		int passed[NUM_PHASES][2]{};
		int isolated[NUM_PHASES][2]{};
		int bishop_pair[NUM_PHASES][2]{};
		int phase{};
		int tempo[2]{};

		void Update(const char* field, Side color, int val_mg, int val_eg)
		{
			if (field == "material") {
				material[MIDGAME][color] += val_mg;
				material[ENDGAME][color] += val_eg;
			}
			else if (field == "mobility") {
				mobility[MIDGAME][color] += val_mg;
				mobility[ENDGAME][color] += val_eg;
			}
			else if (field == "pst") {
				pst[MIDGAME][color] += val_mg;
				pst[ENDGAME][color] += val_eg;
			}
			else if (field == "passed") {
				passed[MIDGAME][color] += val_mg;
				passed[ENDGAME][color] += val_eg;
			}
			else if (field == "isolated") {
				isolated[MIDGAME][color] += val_mg;
				isolated[ENDGAME][color] += val_eg;
			}
			else if (field == "bishop_pair") {
				bishop_pair[MIDGAME][color] += val_mg;
				bishop_pair[ENDGAME][color] += val_eg;
			}
			else if (field == "tempo") {
				tempo[color] += val_mg;
				tempo[color] += val_eg;
			}
			else if (field == "phase") {
				phase = val_mg;
			}
		}

	};

	extern EvalInfo eval_info;
	#endif // EVAL_DEBUG

	void InitPST();
	int ClassicalEvaluation(const GameState& pos);
}