//#include "perft.hpp"
//#include "gamestate.hpp"
//
//static constexpr bool PERFT_TO_6 = true;
//
//struct PerftPosition {
//	const char* fen;
//	u64 d5_count;
//	u64 d6_count;
//};
//
//static constexpr int num_fens = 8;
//static PerftPosition positions[num_fens] = { 
//	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4865609, 119060324},
//	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 193690690, 8031647685},
//	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 674624, 11030083},
//	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 15833292, 706045033},
//	{"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 15833292, 706045033},
//	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ", 89941194, 3048196529},
//	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 164075551, 6923051137},
//	{ "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 3605103, 71179139}
//};
//
//
//int main()
//{
//	using namespace anka;
//
//	RNG rng(719);
//	InitZobristKeys(rng);
//	attacks::InitAttacks();
//
//	GameState pos;
//	constexpr int perft_depth = PERFT_TO_6 ? 6 : 5;
//	int num_success = 0;
//	for (int i = 0; i < num_fens; i++) {
//		pos.LoadPosition(positions[i].fen);
//		std::cout << "Running test " <<i+1 << "... \n";
//		u64 result = perft(pos, perft_depth);
//		u64 expected = PERFT_TO_6 ? positions[i].d6_count : positions[i].d5_count;
//		if (result == expected) {
//			num_success++;
//		}
//		else {
//			std::cout << "FEN " << positions[i].fen << " failed!\n";
//			std::cout << "Perft result: " << result << " Expected value: " << expected << std::endl;
//		}
//	}
//
//	if (num_success != num_fens) {
//		std::cout << "Test(s) failed. ";
//	}
//	else {
//		std::cout << "Tests passed. ";
//	}
//	std::cout << "(" << num_success << " / " << num_fens << ")" << std::endl;
//
//	return 0;
//}