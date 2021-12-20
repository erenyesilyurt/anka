#pragma once

#include "gamestate.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "timer.hpp"
#include "ttable.hpp"


namespace anka {
    bool InitSearch();
    void FreeSearchStack();


    struct SearchResult {
        u64 total_nodes = C64(0);
        long long total_time = 1;
        u64 nps = C64(0);
        int best_score = 0;
        int depth = 0;
        u64 tb_hits = 0;
        Move* pv = nullptr;
        u64 fh = C64(1);
        u64 fh_f = C64(1);

        void Print(GameState& pos, int pv_length) const
        {
            char move_str[6];

            printf("info depth %d time %lld nodes %" PRIu64 " nps %" PRIu64 " tbhits %" PRIu64 " score ",
                depth, total_time, total_nodes,
                nps, tb_hits);

            if (best_score >= UPPER_MATE_THRESHOLD) {
                int moves_to_mate = ((ANKA_MATE - best_score) >> 1) + 1;
                printf("mate %d pv ", moves_to_mate);
            }
            else if (best_score <= LOWER_MATE_THRESHOLD) {
                int moves_to_mate = ((-ANKA_MATE - best_score) >> 1);
                printf("mate %d pv ", moves_to_mate);
            }
            else {
                printf("cp %d pv ", best_score);
            }

            for (int i = 0; i < pv_length; i++) {
                move::ToString(pv[i], move_str);
                printf("%s ", move_str);
            }

            putchar('\n');
            STATS(printf("Order Quality: %.2f\n", fh_f / (float)fh));
        }
    };


    struct SearchParams {
        bool infinite = false;
        long long start_time = 0;
        long long remaining_time = 0;
        int depth_limit = 0;

        bool check_timeup = false;
        std::atomic<bool> uci_stop_flag = false;
        std::atomic<bool> is_searching = false;

        void Clear()
        {
            infinite = false;
            start_time = 0;
            remaining_time = 0;
            depth_limit = 0;

            check_timeup = false;
            uci_stop_flag = false;
            is_searching = false;
        }
    };

    struct SearchStack {
        MoveList<256> move_list[MAX_PLY + 1]{};
    };

	class SearchInstance {
    public:
        int Quiescence(GameState& pos, int alpha, int beta, SearchParams& params);
        template <bool is_pv, bool is_root=false>
        int PVS(GameState& pos, int alpha, int beta, int depth, SearchParams& params);
	public:
        u64 nodes_visited = C64(0);
        u64 tb_hits = 0;
        u64 num_fail_high = C64(1);
        u64 num_fail_high_first = C64(1);
        long long last_timecheck = 0;
        Move root_best_move = move::NO_MOVE;
        bool nmp_enabled = true;
    private:
        void CheckTime(SearchParams& params);
	}; // SearchInstance


    void StartSearch(GameState& root_pos, SearchParams& params);
}