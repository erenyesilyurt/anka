//#include "ttable.hpp"
//#include "rng.hpp"
//#include "hash.hpp"
//#include "attacks.hpp"
//
//int main()
//{
//	using namespace anka;
//
//	constexpr u64 RNG_SEED = 719;
//	anka::RNG rng(RNG_SEED);
//
//	anka::InitZobristKeys(rng);
//	anka::attacks::InitAttacks();
//
//
//	TransposTable ttable;
//	if (!ttable.Init(32)) {
//		fprintf(stderr, "Failed to init table.\n");
//		return 1;
//	}
//
//	constexpr size_t num_iters = 10000000;
//	bool fail = false;
//
//	for (auto i = 0; i < num_iters; i++) {
//		if (fail)
//			break;
//		u64 pos_key = rng.rand64();
//		NodeType type = static_cast<NodeType>(rng.rand64() % 3);
//		int depth = rng.rand64(1, EngineSettings::MAX_DEPTH);
//		int score = rng.rand64(-EngineSettings::MAX_SCORE, EngineSettings::MAX_SCORE);
//		Move best_move = rng.rand32();
//
//		ttable.Set(pos_key, type, depth, best_move, score);
//		TTResult result;
//		ttable.Get(pos_key, result);
//
//		if (result.type != type) {
//			fail = true;
//			fprintf(stderr, "Node type mismatch (stored: %d got: %d)\n", type, result.type);
//		}
//
//		if (result.depth != depth) {
//			fail = true;
//			fprintf(stderr, "Depth mismatch (stored: %d got: %d)\n", depth, result.depth);
//		}
//
//		if (result.score != score) {
//			fail = true;
//			fprintf(stderr, "Score mismatch (stored: %d got: %d)\n", score, result.score);
//		}
//
//		if (result.move != best_move) {
//			fail = true;
//			fprintf(stderr, "Move mismatch (stored: %d got: %d)\n", best_move, result.move);
//		}
//	}
//
//	printf("Transposition table test passed.\n");
//	return 0;
//}