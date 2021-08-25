//#define EVAL_TUNING
//#include "evaluation.hpp"
//#include "timer.hpp"
//
//
//namespace {
//	// Given an evaluation in centipawns, returns the expected outcome of the game (0-1)
//	double Sigmoid(double K, double eval)
//	{
//		return 1.0 / (1.0 + pow(10.0, (-K / 400) * eval));
//	}
//
//
//	double ResultStrToScore(const char* result_str)
//	{
//		double result = 0;
//		if (strcmp("1/2-1/2", result_str) == 0)
//			return 0.5;
//		else if (strcmp("1-0", result_str) == 0)
//			return 1;
//		else if (strcmp("0-1", result_str) == 0)
//			return 0;
//		else {
//			fprintf(stderr, "Invalid result string\n");
//			exit(EXIT_FAILURE);
//		}
//
//	}
//
//	template <int batch_size>
//	struct Batch {
//		static constexpr double ln10 = 2.30258509299;
//		std::vector<anka::GameState> positions;
//		std::vector<double> labels;
//		std::vector<double> evals;
//		double grad = 0.0;
//		
//
//		Batch()
//		{
//			positions.resize(batch_size);
//			evals.resize(batch_size);
//			labels.resize(batch_size);
//		}
//
//		double MSE(double K)
//		{
//			double mse = 0.0;
//			grad = 0.0;
//			for (int i = 0; i < batch_size; i++) {
//				double sigmoid = Sigmoid(K, evals[i]);
//				double pos_error = labels[i] - sigmoid;
//				mse += (pos_error*pos_error) / batch_size;
//
//				grad += (evals[i] / 400.0) * (sigmoid - labels[i]) * sigmoid * (1 - sigmoid);
//			}
//
//			grad = grad * (2 * ln10 / batch_size);
//			return mse;
//		}
//
//		bool Read(FILE* file_stream)
//		{
//			constexpr int BUFFER_SIZE = 4096;
//			char buffer[BUFFER_SIZE];
//
//			for (int i = 0; i < batch_size; i++) {
//				if (fgets(buffer, BUFFER_SIZE, file_stream) == NULL) {
//					fprintf(stderr, "Failed to read position number %d.", i + 1);
//					fclose(file_stream);
//					return false;
//				}
//
//				//char* ptr = strstr(buffer, "c9");
//				char* fen = strtok(buffer, "\"");		
//				char* result_str = strtok(NULL, "\"");
//				*(result_str - 5) = '\0';
//				double game_result = ResultStrToScore(result_str);
//			
//
//				if (!positions[i].LoadPosition(fen)) {
//					fprintf(stderr, "Failed to load position: %s\n", fen);
//					fclose(file_stream);
//					return false;
//				}
//				labels[i] = game_result;
//
//				double eval = static_cast<double>(positions[i].ClassicalEvaluation());
//				evals[i] = positions[i].SideToPlay() == anka::WHITE ? eval : -eval;
//			}
//
//			return true;
//		}
//
//	};
//
//
//
//	void Init(anka::RNG& rng)
//	{
//		using namespace anka;
//		anka::InitZobristKeys(rng);
//		anka::attacks::InitAttacks();
//		anka::eval_params.InitPST();
//	}
//}
//int main()
//{
//	anka::RNG rng;
//	Init(rng);
//	
//	constexpr int NUM_POSITIONS = 725000;
//	const char* filename = "quiet-labeled.epd";
//
//	FILE* pos_file = fopen(filename, "r");
//	if (pos_file == NULL) {
//		fprintf(stderr, "Failed to open %s", filename);
//		return EXIT_FAILURE;
//	}
//
//	printf("Reading position data...\n");
//	Batch<NUM_POSITIONS> batch;
//	if (!batch.Read(pos_file))
//		return EXIT_FAILURE;
//
//	double lr = 0.20;
//	printf("Optimizing K...\n");
//	anka::Timer t;
//	double K = 1.0;
//	double best_k = K;
//	double best_mse = DBL_MAX;
//	int no_improv_count = 0;
//	int stop_threshold = 100;
//	while(true) {
//		double mse = batch.MSE(K);
//		if (mse < best_mse) {
//			no_improv_count = 0;
//			best_mse = mse;
//			best_k = K;
//		}
//		else {
//			no_improv_count++;
//			if (no_improv_count == stop_threshold)
//				break;
//		}
//		printf("K: %f MSE: %f RMSE: %f Grad: %f\n", K, mse, sqrt(mse), batch.grad);
//		K = K - lr * batch.grad;
//	}
//
//	printf("Done. Best K: %f MSE: %f RMSE: %f\n", best_k, best_mse, sqrt(best_mse));
//
//}