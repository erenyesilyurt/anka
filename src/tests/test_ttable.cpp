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
//	TranspositionTable ttable;
//	if (!ttable.Init(35)) {
//		fprintf(stderr, "Failed to init table.\n");
//		return 1;
//	}
//
//	// Test reallocation
//	if (!ttable.Init(27)) {
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
//		int depth = rng.rand64(1, MAX_PLY);
//		int value = rng.rand(-15000, 15000);
//		Move best_move = rng.rand32();
//
//		int iteration_age = (i + 1) & 0x3f;
//		ttable.IncrementAge();
//		assert(ttable.GetAge() == iteration_age);
//
//		ttable.Put(pos_key, type, depth, best_move, value, 0, false);
//		TTRecord result;
//		ttable.Get(pos_key, result, 0);
//
//		if (result.GetNodeType() != type) {
//			fail = true;
//			fprintf(stderr, "Node type mismatch (stored: %d got: %d)\n", type, result.GetNodeType());
//		}
//
//		if (result.depth != depth) {
//			fail = true;
//			fprintf(stderr, "Depth mismatch (stored: %d got: %d)\n", depth, result.depth);
//		}
//
//		if (result.value != value) {
//			fail = true;
//			fprintf(stderr, "Value mismatch (stored: %d got: %d)\n", value, result.value);
//		}
//
//		if (result.move != best_move) {
//			fail = true;
//			fprintf(stderr, "Move mismatch (stored: %d got: %d)\n", best_move, result.move);
//		}
//	}
//
//	if (!fail)
//		printf("Transposition table test passed.\n");
//	return 0;
//}