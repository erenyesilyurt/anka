#pragma once

#include "gamestate.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "timer.hpp"
#include "ttable.hpp"


namespace anka {

    struct HistoryTable
    {
        int history[NUM_SIDES][64][64]{};

        void Clear()
        {
            for (Side color : {WHITE, BLACK}) {
                for (int fr = 0; fr < 64; fr++) {
                    for (int to = 0; to < 64; to++) {
                        history[color][fr][to] = 0;
                    }
                }
            }
        }

        force_inline void Update(Side color, Square from, Square to, int depth) 
        {
            history[color][from][to] += depth * depth;
            if (history[color][from][to] >= move::KILLER_SCORE) {
                history[color][from][to] /= 2;
            }
        }
    };
    inline Move KillerMoves[MAX_PLY][2];
    force_inline void SetKillerMove(Move m, int ply)
    {
        #ifdef ANKA_DEBUG
        if (KillerMoves[ply][0] != 0 && KillerMoves[ply][1] != 0)
            assert(KillerMoves[ply][0] != KillerMoves[ply][1]);
        #endif // ANKA_DEBUG

        if (KillerMoves[ply][0] != m) {
            KillerMoves[ply][1] = KillerMoves[ply][0];
            KillerMoves[ply][0] = m;
        }
    }

    force_inline void ClearKillerMoves()
    {
        for (int i = 0; i < MAX_PLY; i++) {
            KillerMoves[i][0] = move::NO_MOVE;
            KillerMoves[i][1] = move::NO_MOVE;
        }
    }


    struct SearchResult {
        u64 total_nodes = C64(0);
        long long total_time = 1;
        u64 nps = C64(0);
        int best_score = 0;
        int depth = 0;
        Move* pv = nullptr;
        u64 fh = C64(1);
        u64 fh_f = C64(1);

        void Print(GameState& pos) const
        {
            char move_str[6];

            printf("info depth %d time %lld nodes %" PRIu64 " nps %" PRIu64 " score ",
                depth, total_time, total_nodes,
                nps);

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

            for (int i = 0; i < MAX_PLY; i++) {
                if (pv[i] == 0)
                    break;
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

	class SearchInstance {
	public:
        u64 nodes_visited = C64(0);
        u64 num_fail_high = C64(1);
        u64 num_fail_high_first = C64(1);
        long long last_timecheck = 0;
	public:
        int Quiescence(GameState& pos, int alpha, int beta, int ply, SearchParams& params);
        template <bool pruning = true>
        int PVS(GameState& pos, int alpha, int beta, int depth, int ply, bool is_pv, bool null_pruning, SearchParams& params);
    private:
        void CheckTime(SearchParams& params);
	}; // SearchInstance


    void StartSearch(GameState& root_pos, SearchParams& params);
}