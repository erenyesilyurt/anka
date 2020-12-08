#include "bitboard.hpp"
#include "attacks.hpp"
#include "rng.hpp"

#include <array>
#include <vector>
#include <algorithm>
#include <numeric>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

using namespace anka;


u64 FindMagic(Square square, bool is_rook, bool eval_for_size = true, unsigned int max_iterations = 100)
{
	RNG rng;
	u64 best_magic = C64(0);
	int best_score = -1;

	static u64 buckets[4096]{};
	static u64 occupations[4096]{};
	static u64 piece_attacks[4096]{};

	auto mask = is_rook ? attacks::magics::rook_magics[square].mask
		: attacks::magics::bishop_magics[square].mask;
	int shift = is_rook ? (64 - 12) : (64 - 9);

	// generate all possible relevant occupations and corresponding attacks
	int num_mask_bits = bitboard::PopCount(mask);
	int num_occupations = (1 << num_mask_bits);
	for (Bitboard selection = 0; selection < num_occupations; selection++) {
		occupations[selection] = bitboard::BitSubset(mask, selection);
		piece_attacks[selection] = is_rook ? attacks::RookAttacksSlow(square, occupations[selection])
			: attacks::BishopAttacksSlow(square, occupations[selection]);
	}


	for (int i = 0; (i < max_iterations) || (best_magic == 0); i++) {
		u64 magic = rng.rand64_sparse();
		if (bitboard::PopCount((magic * mask) & C64(0xFF00000000000000)) < 6)
			continue;
		memset(buckets, 0, 4096 * sizeof(u64));
		int max_index = -1; // max index for this magic
		bool success = true;
		for (int j = 0; j < num_occupations; j++) {
			auto index = static_cast<int>((occupations[j] * magic) >> shift);
			if (index > max_index)
				max_index = index;

			if (buckets[index] == 0) {
				buckets[index] = piece_attacks[j];
			}
			else if (buckets[index] != piece_attacks[j]) {
				success = false;
				break; // index collision
			}
		}

		if (success) {
			int score = -1;
			if (eval_for_size) {
				score = 4095 - max_index;
			}
			else { // eval for biggest gap
				int j = 0;
				for (j = 0; j < num_occupations; j++) {
					if (buckets[j] != 0)
						break;
				}
				int previous = j;
				for (j = j + 1; j < num_occupations; j++) {
					if (buckets[j] == 0)
						continue;
					int gap = j - previous;
					previous = j;
					if (gap > score)
						score = gap;
				}

			}

			if (score > best_score) {
				best_score = score;
				best_magic = magic;
			}
		}
	}

	//std::cout << best_magic << ' ' << best_score << '\n';
	return best_magic;
}

struct Solution
{
	//int packing_order[128];
	//int chosen_candidates[128];

	//void Generate(RNG &rng)
	//{
	//	for (int j = 0; j < 128; j++) {
	//		packing_order[j] = j;
	//		chosen_candidates[j] = static_cast<int>(rng.rand64(0, 2));
	//	}
	//	std::shuffle(std::begin(packing_order), std::end(packing_order), rng);
	//}


	double keys[256]; 

	void Generate(RNG& rng)
	{
		for (int i = 0; i < 256; i++) {
			keys[i] = rng.rand();
		}
	}

	void Decode(std::array<int, 128>& packing_order, std::array<int, 128>& choices)
	{
		// 0-127: packing order
		// 128-255: candidate choices
		std::iota(packing_order.begin(), packing_order.end(), 0);
		std::sort(packing_order.begin(), packing_order.end(), [this](int a, int b) {return keys[a] < keys[b]; });

		for (int i = 128; i < 256; i++) {
			if (keys[i] < 0.3333333) {
				choices[i - 128] = 0;
			}
			else if (keys[i] < 0.6666666) {
				choices[i - 128] = 1;
			}
			else {
				choices[i - 128] = 2;
			}
		}
	}

	int Evaluate(u64 (*magics)[3])
	{
		
		int cost = 0;

		std::array<int, 128> packing_order;
		std::array<int, 128> choices;
		Decode(packing_order, choices);


		static byte gtable[4096 * 128]{};
		static int indices[4096]{};

		memset(gtable, 0, 4096 * 128);
		for (int i = 0; i < 128; i++) {
			int magic_index = packing_order[i];
			bool is_rook = magic_index < 64 ? true : false;
			Square square = is_rook ? magic_index : magic_index - 64;
			u64 magic = magics[magic_index][choices[i]];

			auto mask = is_rook ? attacks::magics::rook_magics[square].mask
				: attacks::magics::bishop_magics[square].mask;
			int shift = is_rook ? (64 - 12) : (64 - 9);
			
			// generate all possible local table indices for square and piece type
			int num_mask_bits = bitboard::PopCount(mask);
			int num_occupations = (1 << num_mask_bits);
			
			for (int j = 0; j < num_occupations; j++) {
				indices[j] = (bitboard::BitSubset(mask, j) * magic) >> shift;
			}

			int offset = 0;
			while (true) {
				bool success = true;
				for (int j = 0; j < num_occupations; j++) {
					if (gtable[offset + indices[j]] != 0) {
						success = false;
						break;
					}
				}
				if (success) {
					break;
				}
				else {
					offset++;
				}
			}

			for (int j = 0; j < num_occupations; j++) {
				gtable[offset + indices[j]] = 1;
			}

			if (offset > cost)
				cost = offset;
		}


		return cost;
	}
};

int main()
{
	u64 magics[128][3]; // 0-63: rook magics, 64-127: bishop magics
	
	constexpr unsigned int num_iterations = 10000000;
	printf("Generating candidate rook magics...\n");
	for (int sq = 0; sq < 64; sq++) {
		magics[sq][0] = FindMagic(sq, true, true, num_iterations);
		magics[sq][1] = FindMagic(sq, true, false, num_iterations);
		magics[sq][2] = FindMagic(sq, true, false, num_iterations);
	}
	printf("Generating candidate bishop magics...\n");
	for (int sq = 0; sq < 64; sq++) {
		magics[64 + sq][0] = FindMagic(sq, false, true, num_iterations);
		magics[64 + sq][1] = FindMagic(sq, false, false, num_iterations);
		magics[64 + sq][2] = FindMagic(sq, false, false, num_iterations);
	}

	printf("Optimizing the packing problem...\n");
	
	RNG rng;
	// BRKGA parameters
	constexpr int population_size = 100;
	constexpr int n_genes = 256;
	constexpr int n_elites = 5;
	constexpr int n_mutated = 25;
	constexpr int n_nonelites = population_size - n_elites - n_mutated;
	constexpr double pe = 0.5; // probability that the offspring will inherit a gene from the elite parent
	constexpr int n_generations = 10;

	// generate the initial population
	std::vector<Solution> population(population_size);
	for (auto &solution : population) {
		solution.Generate(rng);
	}

	std::vector<Solution> tmp_pop(population_size);
	int costs[population_size];
	int positions[population_size];
	std::iota(std::begin(positions), std::end(positions), 0);
	for (int n = 1; n <= n_generations; n++) {
		// 1. Evaluate and sort the current population positions
		for (int i = 0; i < population_size; i++) {
			costs[i] = population[i].Evaluate(magics);
		}
		std::stable_sort(std::begin(positions), std::end(positions), [costs](int p1, int p2) { return costs[p1] < costs[p2]; });

		// 2. carry over the elite individuals
		for (int i = 0; i < n_elites; i++) {
			tmp_pop[i] = population[positions[i]];
		}

		// 3. Mating with uniform crossover	
		for (int j = 0; j < n_nonelites; j++) {
			int parent_1_index = rng.rand64(0, n_elites - 1);
			int parent_2_index = rng.rand64(n_elites, population_size - 1);
			const Solution& parent_1 = tmp_pop[parent_1_index];;
			const Solution& parent_2 = population[positions[parent_2_index]];		
			for (int i = 0; i < n_genes; i++) {
				double p = rng.rand();
				tmp_pop[n_elites + j].keys[i] = p < pe ? parent_1.keys[i] : parent_2.keys[i];
			}
		}

		// 4. Generate randomly mutated solutions
		for (int j = 0; j < n_mutated; j++) {
			tmp_pop[n_elites + n_nonelites + j].Generate(rng);
		}

		population.swap(tmp_pop);
		printf("Gen %d best: %d KiB\n", n, (costs[0] + 4096) / 128);
		
	}

	return 0;
}