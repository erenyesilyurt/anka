#pragma once

#include "gamestate.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "timer.hpp"
#include "ttable.hpp"


namespace anka {

    inline constexpr int num_killers_per_slot = 2;
    inline Move KillerMoves[MAX_PLY][num_killers_per_slot];

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
            STATS(printf("Order Quality: %.2f\n", result.fh_f / (float)result.fh));
        }
    };

	class SearchInstance {
	public:
        u64 nodes_visited = C64(0);
        u64 num_fail_high = C64(1);
        u64 num_fail_high_first = C64(1);
        long long last_timecheck = 0;
	public:
        int Quiescence(GameState& pos, int alpha, int beta, int depth, int ply, SearchParams& params);
        template <bool pruning = true>
        int PVS(GameState& pos, int alpha, int beta, int depth, int ply, bool is_pv, SearchParams& params);
    private:
        void CheckTime(SearchParams& params);
	}; // SearchInstance




    void IterativeDeepening(GameState& root_pos, SearchParams& params);
}