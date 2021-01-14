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
#include <thread>
#include <future>

using namespace anka;

static u64 bishop_masks[64] =
{
	0x0040201008040200,
	0x0000402010080400,
	0x0000004020100a00,
	0x0000000040221400,
	0x0000000002442800,
	0x0000000204085000,
	0x0000020408102000,
	0x0002040810204000,
	0x0020100804020000,
	0x0040201008040000,
	0x00004020100a0000,
	0x0000004022140000,
	0x0000000244280000,
	0x0000020408500000,
	0x0002040810200000,
	0x0004081020400000,
	0x0010080402000200,
	0x0020100804000400,
	0x004020100a000a00,
	0x0000402214001400,
	0x0000024428002800,
	0x0002040850005000,
	0x0004081020002000,
	0x0008102040004000,
	0x0008040200020400,
	0x0010080400040800,
	0x0020100a000a1000,
	0x0040221400142200,
	0x0002442800284400,
	0x0004085000500800,
	0x0008102000201000,
	0x0010204000402000,
	0x0004020002040800,
	0x0008040004081000,
	0x00100a000a102000,
	0x0022140014224000,
	0x0044280028440200,
	0x0008500050080400,
	0x0010200020100800,
	0x0020400040201000,
	0x0002000204081000,
	0x0004000408102000,
	0x000a000a10204000,
	0x0014001422400000,
	0x0028002844020000,
	0x0050005008040200,
	0x0020002010080400,
	0x0040004020100800,
	0x0000020408102000,
	0x0000040810204000,
	0x00000a1020400000,
	0x0000142240000000,
	0x0000284402000000,
	0x0000500804020000,
	0x0000201008040200,
	0x0000402010080400,
	0x0002040810204000,
	0x0004081020400000,
	0x000a102040000000,
	0x0014224000000000,
	0x0028440200000000,
	0x0050080402000000,
	0x0020100804020000,
	0x0040201008040200
};

static u64 rook_masks[64] =
{
	0x000101010101017e,
	0x000202020202027c,
	0x000404040404047a,
	0x0008080808080876,
	0x001010101010106e,
	0x002020202020205e,
	0x004040404040403e,
	0x008080808080807e,
	0x0001010101017e00,
	0x0002020202027c00,
	0x0004040404047a00,
	0x0008080808087600,
	0x0010101010106e00,
	0x0020202020205e00,
	0x0040404040403e00,
	0x0080808080807e00,
	0x00010101017e0100,
	0x00020202027c0200,
	0x00040404047a0400,
	0x0008080808760800,
	0x00101010106e1000,
	0x00202020205e2000,
	0x00404040403e4000,
	0x00808080807e8000,
	0x000101017e010100,
	0x000202027c020200,
	0x000404047a040400,
	0x0008080876080800,
	0x001010106e101000,
	0x002020205e202000,
	0x004040403e404000,
	0x008080807e808000,
	0x0001017e01010100,
	0x0002027c02020200,
	0x0004047a04040400,
	0x0008087608080800,
	0x0010106e10101000,
	0x0020205e20202000,
	0x0040403e40404000,
	0x0080807e80808000,
	0x00017e0101010100,
	0x00027c0202020200,
	0x00047a0404040400,
	0x0008760808080800,
	0x00106e1010101000,
	0x00205e2020202000,
	0x00403e4040404000,
	0x00807e8080808000,
	0x007e010101010100,
	0x007c020202020200,
	0x007a040404040400,
	0x0076080808080800,
	0x006e101010101000,
	0x005e202020202000,
	0x003e404040404000,
	0x007e808080808000,
	0x7e01010101010100,
	0x7c02020202020200,
	0x7a04040404040400,
	0x7608080808080800,
	0x6e10101010101000,
	0x5e20202020202000,
	0x3e40404040404000,
	0x7e80808080808000
};


/**
* Find a magic factor.
* @param rng random number generator.
* @param square the square corresponding to the magic factor.
* @param is_rook true if the factor is for a rook, false if it is for a bishop.
* @param eval_for_size true: smallest size, false: largest gaps
* @param max_tries maximum number of iterations to find a better magic. Can be exceeded if no magic is yet found.
* @return A magic factor.
*/
u64 FindMagic(Square square, bool is_rook, bool eval_for_size, unsigned int max_tries)
{
	RNG rng;
	u64 best_magic = C64(0);
	int lowest_cost = INT32_MAX;

	thread_local u64 buckets[4096]{};
	thread_local u64 occupations[4096]{};
	thread_local u64 piece_attacks[4096]{};

	auto mask = is_rook ? rook_masks[square]
		: bishop_masks[square];
	int shift = is_rook ? (64 - 12) : (64 - 9);

	// generate all possible relevant occupations and corresponding attacks
	int num_mask_bits = bitboard::PopCount(mask);
	int num_occupations = (1 << num_mask_bits);
	for (Bitboard selection = 0; selection < num_occupations; selection++) {
		occupations[selection] = bitboard::BitSubset(mask, selection);
		piece_attacks[selection] = is_rook ? attacks::RookAttacksSlow(square, occupations[selection])
			: attacks::BishopAttacksSlow(square, occupations[selection]);
	}


	for (unsigned int i = 0; (i < max_tries) || (best_magic == 0); i++) {
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

		// evaluate found magic
		if (success) {
			int cost = 0;
			if (eval_for_size) {
				cost = max_index;
			}
			else { // eval for gaps
				int space = 0;
				int large_gap_sum = 0;
				int num_gaps = 0;
				int threshold = 16;
				for (int j = 0; j < num_occupations; j++) {
					if (buckets[j] != 0) {
						if (space > threshold) {
							num_gaps++;
							large_gap_sum += space;
						}
						space = 0;
					}
					else {
						space++;
					}
				}

				cost = max_index - 2*large_gap_sum + 10*num_gaps;
			}

			if (cost < lowest_cost) {
				lowest_cost = cost;
				best_magic = magic;
			}
		}
	}

	return best_magic;
}

/**
* A possible solution to the optimization problem.
*/
struct Individual
{
	/**
	* An array encoding the magic factor choices for rooks from the available magic triplets.
	* Indexes indicate the square.
	* Possible values are 0, 1, and 2; indicating the location of the chosen magic factor.
	* For example, choices[0] = 2 implies that the third magic factor for the A1 rook must be chosen.
	*/
	std::array<int, 64> rook_choices;

	/**
	* The insertion order of the magic factors into the global table.
	* This is a permutation of the sequence of integers from 0 to 127.
	*/
	std::array<int, 128> packing_order;


	/**
	* Randomly generate a new individual.
	* @param rng a reference to the random number generator.
	*/
	void Generate(RNG& rng)
	{
		std::iota(packing_order.begin(), packing_order.end(), 0);
		std::shuffle(packing_order.begin(), packing_order.end(), rng);

		for (int i = 0; i < 64; i++) {
			rook_choices[i] = rng.rand64(0, 2);
		}	
	}

	/**
	* Evaluate the fitness of the individual.
	* @param rook_magics rook magic triplets table.
	* @param bishop_magics bishop magics table.
	* @param offsets if not nullptr, stores offsets here.
	* @return The fitness (in this case, the cost) of the solution.
	*/
	int Evaluate(const u64(*rook_magics)[64], const u64* bishop_magics, std::array<int, 128>* offsets = nullptr) const
	{
		int cost = 0; // max offset

		thread_local byte gtable[4096 * 128]{};
		thread_local int indices[4096]{};

		memset(gtable, 0, 4096 * 128);
		for (int i = 0; i < 128; i++) {
			int magic_index = packing_order[i];
			bool is_rook = magic_index < 64 ? true : false;
			Square square;
			u64 mask, magic;
			int shift;
			
			if (is_rook) {
				square = magic_index;
				mask = rook_masks[square];
				shift = (64 - 12);
				int chosen_magic_index = rook_choices[square];
				magic = rook_magics[chosen_magic_index][square];
			}
			else {
				square = magic_index - 64;
				mask = bishop_masks[square];
				shift = (64 - 9);
				magic = bishop_magics[square];
			}


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

			if (offsets != nullptr) {
				(*offsets)[magic_index] = offset;
			}

			for (int j = 0; j < num_occupations; j++) {
				gtable[offset + indices[j]] = 1;
			}

			if (offset > cost)
				cost = offset;
		}


		return cost;
	}

	/**
	* Mutate the individual.
	* @param rng random number generator.
	* @param pm mutation probability.
	*/
	void Mutate(RNG& rng, double pm)
	{
		using std::swap;
		for (int i = 0; i < 128; i++) {
			double p = rng.rand();
			if (p < pm) {
				int j = rng.rand64(0, 127);
				swap(packing_order[i], packing_order[j]);
			}
		}

		for (auto& choice : rook_choices) {
			double p = rng.rand();
			if (p < pm)
				choice = rng.rand64(0, 2);
		}
	}

	/**
	* Creates two children via crossover between two parents.
	* The crossover is performed over either the packing_order or the  choices arrays with %50 chance.
	* @param parent_1 the first parent individual.
	* @param parent_2 the second parent individual.
	* @param child_1 the first child created by crossover.
	* @param child_2 the second child created by crossover.
	* @param rng random number generator.
	*/
	static void Crossover(const Individual& parent_1, const Individual& parent_2, Individual& child_1, Individual& child_2, RNG& rng)
	{

		// Order crossover (OX) for packing_order
		int a = static_cast<int>(rng.rand64(0, 126));
		int b = static_cast<int>(rng.rand64(a + 1, 127)) + 1;

		byte child_1_marked[128]{};
		byte child_2_marked[128]{};
		for (int i = a; i < b; i++) {
			int val = parent_1.packing_order[i];
			child_1.packing_order[i] = val;
			child_1_marked[val] = 1;

			val = parent_2.packing_order[i];
			child_2.packing_order[i] = val;
			child_2_marked[val] = 1;
		}

		int j = 0;
		for (int i = 0; i < a; i++) {
			while (j < 128) {
				int val = parent_2.packing_order[j++];
				if (child_1_marked[val])
					continue;
				child_1.packing_order[i] = val;
				break;
			}
		}

		for (int i = b; i < 128; i++) {
			while (j < 128) {
				int val = parent_2.packing_order[j++];
				if (child_1_marked[val])
					continue;
				child_1.packing_order[i] = val;
				break;
			}
		}

		j = 0;
		for (int i = 0; i < a; i++) {
			while (j < 128) {
				int val = parent_1.packing_order[j++];
				if (child_2_marked[val])
					continue;
				child_2.packing_order[i] = val;
				break;
			}
		}

		for (int i = b; i < 128; i++) {
			while (j < 128) {
				int val = parent_1.packing_order[j++];
				if (child_2_marked[val])
					continue;
				child_2.packing_order[i] = val;
				break;
			}
		}

		// single point crossover for rook choices
		a = static_cast<int>(rng.rand64(0, 64));
		for (int i = 0; i < a; i++) {
			child_1.rook_choices[i] = parent_1.rook_choices[i];
			child_2.rook_choices[i] = parent_2.rook_choices[i];
		}

		for (int i = a; i < 64; i++) {
			child_1.rook_choices[i] = parent_2.rook_choices[i];
			child_2.rook_choices[i] = parent_1.rook_choices[i];
		}
	}


	/**
	* Write the solution into a file, formatted as C++ code.
	* @param filename name of the output file.
	* @param rook_magics rook magic triplets
	* @param bishop_magics bishop magics
	*/
	void WriteToFile(const char* filename, const u64(*rook_magics)[64], const u64* bishop_magics) const
	{
		FILE* file = fopen(filename, "w");


		if (!file) {
			fprintf(stderr, "%s could not be opened.", filename);
			return;
		}

		std::array<int, 128> offsets;
		offsets.fill(-1);
		Evaluate(rook_magics, bishop_magics, &offsets);

		fprintf(file, "inline constexpr Magic rook_magics[64] =\n{\n");
		for (int i = 0; i < 64; i++) {
			fprintf(file, "\t{ 0x%" PRIx64 ", 0x%" PRIx64 ",  %d },\n", rook_masks[i], rook_magics[rook_choices[i]][i], offsets[i]);
		}
		fprintf(file, "}\n");


		fprintf(file, "inline constexpr Magic bishop_magics[64] =\n{\n");
		for (int i = 0; i < 64; i++) {
			fprintf(file, "\t{ 0x%" PRIx64 ", 0x%" PRIx64 ",  %d },\n", bishop_masks[i], bishop_magics[i], offsets[64 + i]);
		}
		fprintf(file, "}\n");
	}
};


/**
* Convert the passed rank to a fitness value for rank-based selection.
* @param rank The ranking of the individual. Most fit (lowest cost) individual has the rank 0.
* @param sp the selection pressure, must be a value between 1 and 2.
* @param pop_size the number of individuals in the population.
* @return The fitness of the individual calculated from its rank.
*/
constexpr double RankToFitness(int rank, double sp, int pop_size)
{
	rank = pop_size - rank; // reverse rank so that the most fit individual has the highest rank
	return 2 - sp + (2 * (sp - 1) * (rank - 1) / (pop_size - 1));
}

/**
* Roulette wheel selection.
* @tparam n the population size.
* @param rng a reference to the random number generator.
* @param cum_fitness the array of cumulative fitness values.
* @return the rank of the chosen individual.
*/
template <int n>
int SpinWheel(RNG& rng, const std::array<double, n>& cum_fitness)
{
	int chosen_one = n - 1;
	double roll = rng.rand() * cum_fitness[n - 1];
	for (int i = 0; i < n; i++) {
		if (roll < cum_fitness[i]) {
			chosen_one = i;
			break;
		}
	}
	return chosen_one;
}

int main()
{
	constexpr int num_threads = 8;
	RNG rng;

	u64 rook_magics[3][64];
	u64 bishop_magics[64];

	constexpr unsigned int num_tries = 500000000;
	
	printf("Generating rook magics...\n");
	std::future<u64> magic_futures[num_threads];
	for (int m = 0; m < 3; m++) {
		bool eval_for_size = (m == 0 ? true : false);
		
			
		for (int sq = 0; sq < 64; sq+=num_threads) {
			for (int t = 0; t < num_threads; t++) {
				magic_futures[t] = std::async(std::launch::async, [=] { return FindMagic(sq + t, true, eval_for_size, num_tries); });
			}

			for (int t = 0; t < num_threads; t++) {
				rook_magics[m][sq + t] = magic_futures[t].get();
			}
		}
	}

	printf("Generating bishop magics...\n");
	for (int sq = 0; sq < 64; sq += num_threads) {
		for (int t = 0; t < num_threads; t++) {
			magic_futures[t] = std::async(std::launch::async, [=] { return FindMagic(sq + t, false, true, num_tries); });
		}
		for (int t = 0; t < num_threads; t++) {
			bishop_magics[sq + t] = magic_futures[t].get();
		}
	}
		


	printf("Optimizing the packing problem...\n");




	constexpr int n_generations = 2000;
	constexpr int population_size = 128;
	constexpr int n_elites = 2;
	constexpr int n_nonelites = population_size - n_elites;
	constexpr double sp = 1.7;
	constexpr double p_mut = 0.02;
	constexpr int n_early_term = 120;

	// generate the initial population
	std::vector<Individual> population(population_size);
	for (auto& solution : population) {
		solution.Generate(rng);
	}

	// fill fitness and cumulative fitness values for rank-based selection
	std::array<double, population_size> fitness;
	std::array<double, population_size> cum_fitness;
	fitness[0] = RankToFitness(0, sp, population_size);
	cum_fitness[0] = fitness[0];
	for (int r = 1; r < population_size; r++) {
		fitness[r] = RankToFitness(r, sp, population_size);
		cum_fitness[r] = cum_fitness[r - 1] + fitness[r];
	}


	std::vector<Individual> tmp_pop(population_size);
	int evals[population_size];

	int ordered_indices[population_size]; // contains the indices of individuals in the population array. sorted and indexed by rank.
	std::iota(std::begin(ordered_indices), std::end(ordered_indices), 0);

	int n_no_improvement = 0;
	int previous_best = INT32_MAX;
	constexpr int pop_per_thread = population_size / num_threads;
	std::thread threads[num_threads];
	// THE GENETIC ALGORITHM MAIN LOOP
	for (int n = 1; n <= n_generations; n++) {
		// 1. Evaluate the population and sort ordered_indices
		for (int t = 0; t < num_threads; t++) {
			threads[t] = std::thread([=, &population, &evals, &rook_magics, &bishop_magics]() {
				for (int i = 0; i < pop_per_thread; i++) {
					evals[t * pop_per_thread + i] = population[t * pop_per_thread + i].Evaluate(rook_magics, bishop_magics);
				}
			});
		}
		for (int t = 0; t < num_threads; t++) {
			threads[t].join();
		}

		std::stable_sort(std::begin(ordered_indices), std::end(ordered_indices), [evals](int p1, int p2) { return evals[p1] < evals[p2]; });
		int best_score = evals[ordered_indices[0]];
		printf("Gen %d best: %d KiB\n", n, best_score / 128);
		if (best_score < previous_best) {
			n_no_improvement = 0;
		}
		else {
			n_no_improvement++;
		}
		previous_best = best_score;

		if (n_no_improvement > n_early_term)
			break;

		// 2. carry over the elite individuals
		for (int i = 0; i < n_elites; i++) {
			tmp_pop[i] = population[ordered_indices[i]];
		}

		// 3. Crossover
		for (int i = 0; i < n_nonelites; i += 2) {
			int chosen_one = SpinWheel<population_size>(rng, cum_fitness);
			const Individual& parent_1 = population[ordered_indices[chosen_one]];
			chosen_one = SpinWheel<population_size>(rng, cum_fitness);
			const Individual& parent_2 = population[ordered_indices[chosen_one]];

			Individual::Crossover(parent_1, parent_2, tmp_pop[n_elites + i], tmp_pop[n_elites + i + 1], rng);
		}

		// 4. Mutation
		for (int i = 0; i < n_nonelites; i++) {
			tmp_pop[n_elites + i].Mutate(rng, p_mut);
		}

		population.swap(tmp_pop);
	}

	population[ordered_indices[0]].WriteToFile("found_magics.txt", rook_magics, bishop_magics);

	return 0;
}