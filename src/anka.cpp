#include "mainloop.hpp"
#include "hash.hpp"
#include "rng.hpp"
#include "attacks.hpp"
#include "movegen.hpp"
#include <thread>


int main()
{
	using namespace anka;
	constexpr u64 RNG_SEED = 719;
	anka::RNG rng(RNG_SEED);

	anka::InitZobristKeys(rng);
	anka::attacks::InitAttacks();

	anka::UciLoop();

	//GameState pos;
	//pos.LoadPosition("r4rk1/ppq2ppp/3p1n2/4n3/2b1P3/1NP3P1/PQ1B2BP/3RK2R b K - 1 19");
	//MoveList<256> list;
	//list.GenerateLegalMoves(pos);

	//list.PrintMoves();
	return 0;
}