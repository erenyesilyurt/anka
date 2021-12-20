#include "mainloop.hpp"
#include "hash.hpp"
#include "rng.hpp"
#include "attacks.hpp"
#include "evaluation.hpp"
#include "engine_settings.hpp"
#include "search.hpp"
#include "tbprobe.h"

namespace anka {
	// Global structures
	TranspositionTable g_trans_table;
	EvalParams g_eval_params;
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
	
	if (!g_trans_table.Init(EngineSettings::DEFAULT_HASH_SIZE)) {
		fprintf(stderr, "Failed to allocate transposition table memory\n");
		return EXIT_FAILURE;
	}

	if (!InitSearch()) {
		return EXIT_FAILURE;
	}

	uci::UciLoop();

	tb_free();
	FreeSearchStack();
	return 0;
}