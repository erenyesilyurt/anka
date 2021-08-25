#include "mainloop.hpp"
#include "hash.hpp"
#include "rng.hpp"
#include "attacks.hpp"
#include "movegen.hpp"
#include "evaluation.hpp"
#include <thread>

static void Init()
{
	// unbuffered output
	setbuf(stdout, NULL);

	constexpr u64 RNG_SEED = 6700417;
	anka::RNG rng(RNG_SEED);
	anka::InitZobristKeys(rng);
	anka::attacks::InitAttacks();
	anka::eval_params.InitPST();
}

int main()
{
	Init();
	anka::UciLoop();

	return 0;
}