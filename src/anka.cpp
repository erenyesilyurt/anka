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

	constexpr u64 RNG_SEED = 719;
	anka::RNG rng(RNG_SEED);
	anka::InitZobristKeys(rng);
	anka::attacks::InitAttacks();
	anka::InitPST();
}

int main()
{
	Init();
	anka::UciLoop();

	return 0;
}