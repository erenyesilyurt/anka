//#define EVAL_TUNING
//#define _USE_MATH_DEFINES
//
//#include "evaluation.hpp"
//#include "mainloop.hpp"
//#include "timer.hpp"
//#include "util.hpp"
//#include <numeric>
//#include <math.h>
//#include <float.h>
//#include <algorithm>
//
//
//
//static constexpr double K = 1.00;
//static constexpr double SIG_CONSTANT = (-K / 400.0);
//
//typedef std::array<int, anka::EvalParams::NUM_PARAMS> Solution;
//
//void Init(anka::RNG& rng)
//{
//	using namespace anka;
//	anka::InitZobristKeys(rng);
//	anka::attacks::InitAttacks();
//	anka::g_eval_params.InitPST();
//	anka::g_trans_table.Init(EngineSettings::DEFAULT_HASH_SIZE);
//}
//
//namespace anka {
//	// Global structures
//	TranspositionTable g_trans_table;
//	EvalParams g_eval_params;
//}
//
//// White respective evaluation of the position
//double Evaluate(const anka::GameState& pos)
//{
//	double eval = static_cast<double>(pos.ClassicalEvaluation());
//	return pos.SideToPlay() == anka::WHITE ? eval : -eval;
//}
//
//
//double Sigmoid(double eval) 
//{
//	return 1.0 / (1.0 + pow(10.0, SIG_CONSTANT * eval));
//}
//
//
//double ResultStrToScore(const char* result_str)
//{
//	double result = 0;
//	if (strcmp("1/2-1/2", result_str) == 0)
//		return 0.5;
//	else if (strcmp("1-0", result_str) == 0)
//		return 1;
//	else if (strcmp("0-1", result_str) == 0)
//		return 0;
//	else {
//		fprintf(stderr, "Invalid result string\n");
//		exit(EXIT_FAILURE);
//	}
//
//}
//
//
//template <int batch_size>
//struct Batch {
//	std::vector<anka::GameState> positions;
//	std::vector<double> labels;
//
//	Batch()
//	{
//		positions.resize(batch_size);
//		labels.resize(batch_size);
//	}
//
//
//	bool Read(FILE* file_stream)
//	{
//		constexpr int BUFFER_SIZE = 4096;
//		char buffer[BUFFER_SIZE];
//
//		for (int i = 0; i < batch_size; i++) {
//			if (fgets(buffer, BUFFER_SIZE, file_stream) == NULL) {
//				fprintf(stderr, "Failed to read position number %d.", i + 1);
//				fclose(file_stream);
//				return false;
//			}
//
//			char* fen = strtok(buffer, "\"");
//			char* result_str = strtok(NULL, "\"");
//			*(result_str - 5) = '\0';
//			double game_result = ResultStrToScore(result_str);
//
//			if (!positions[i].LoadPosition(fen)) {
//				fprintf(stderr, "Failed to load position: %s\n", fen);
//				fclose(file_stream);
//				return false;
//			}
//			labels[i] = game_result;
//		}
//
//		return true;
//	}
//
//	double MSE(const Solution &solution)
//	{
//		anka::g_eval_params.UpdateParams(solution);
//		double mse = 0.0;
//		for (int i = 0; i < batch_size; i++) {
//			double sigmoid = Sigmoid(Evaluate(positions[i]));
//			double pos_error = labels[i] - sigmoid;
//			mse += (pos_error * pos_error) / batch_size;
//		}
//		return mse;
//	}
//};
//
//
//template <int batch_size>
//static void GreedyLocalSearch(anka::RNG& rng, Batch<batch_size>* batch, int num_iterations = 1000000)
//{
//	using namespace anka;
//	
//	Solution best_sol;
//	g_eval_params.FlattenParams(best_sol);
//	double best_cost = batch->MSE(best_sol);
//
//	printf("Itr 0 MSE: %f RMSE: %f\n", best_cost, sqrt(best_cost));
//	std::uniform_int_distribution<> step_dist(-2, 2);
//	int step = 0;
//	int itr = 0;
//
//	while (itr < num_iterations) {
//		itr++;
//		int rand_param_index = rng.randint(0, EvalParams::NUM_PARAMS);
//		do {
//			step = step_dist(rng);
//		} while (step == 0);
//
//		best_sol[rand_param_index] += step;
//		double cost = batch->MSE(best_sol);
//		
//		if (cost < best_cost) {
//			best_cost = cost;
//		}
//		else {
//			best_sol[rand_param_index] -= step;
//		}
//
//
//		if (itr % 1000 == 0) {
//			printf("Itr %d MSE: %f RMSE: %f\n", itr, best_cost, sqrt(best_cost));
//			char filename[128];
//			sprintf(filename, "best_params/best_params725K_LOCAL%d.txt", itr);
//			anka::g_eval_params.UpdateParams(best_sol);
//			anka::g_eval_params.WriteParamsToFile(filename);
//		}
//		else if (itr % 50 == 0) {
//			printf("Itr %d MSE: %f RMSE: %f\n", itr, best_cost, sqrt(best_cost));
//		}
//
//	}
//
//	printf("\nDone. MSE: %f RMSE: %f\n", best_cost, sqrt(best_cost));
//	g_eval_params.UpdateParams(best_sol);
//	g_eval_params.WriteParamsToFile("best_params/best_params725K_LOCAL.txt");
//}
//
//
//
//int main()
//{
//	anka::RNG rng;
//	Init(rng);
//
//	constexpr int TRAINING_DATA_SIZE = 725'000;
//	const char* filename_training = R"(quiet-labeled.epd)";
//
//	FILE* training_file = fopen(filename_training, "r");
//	if (training_file == NULL) {
//		fprintf(stderr, "Failed to open %s", filename_training);
//		return EXIT_FAILURE;
//	}
//
//	Batch<TRAINING_DATA_SIZE> training_set;
//	
//	printf("Reading the dataset...\n");
//	if (!training_set.Read(training_file)) {
//			fclose(training_file);
//			return EXIT_FAILURE;
//	}
//	fclose(training_file);
//
//
//	anka::Timer t;
//	printf("Starting Optimization...\n");	
//	GreedyLocalSearch<TRAINING_DATA_SIZE>(rng, &training_set);
//
//
//	return 0;
//}