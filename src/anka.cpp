#include "mainloop.hpp"
#include "hash.hpp"
#include "rng.hpp"
#include "attacks.hpp"
#include "evaluation.hpp"
#include "engine_settings.hpp"

namespace anka {
	// Global structures
	TranspositionTable g_trans_table;
	EvalParams g_eval_params;
	EvalData g_eval_data;

	void InitLMR();
}

int main()
{
	using namespace anka;
	
	setbuf(stdout, NULL); // unbuffered output
	constexpr u64 RNG_SEED = 6700417;
	RNG rng(RNG_SEED);

	
	InitZobristKeys(rng);
	attacks::InitAttacks();

	g_eval_params.InitPST();
	g_trans_table.Init(EngineSettings::DEFAULT_HASH_SIZE);
	InitLMR();

	uci::UciLoop();
	return 0;
}