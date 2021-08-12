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
    };

	class SearchInstance {
	public:
        u64 nodes_visited = C64(0);
        u64 num_fail_high = C64(1);
        u64 num_fail_high_first = C64(1);
        long long last_timecheck = 0;
	public:
        int Quiescence(GameState& pos, int alpha, int beta, int depth, int ply, SearchParams& params);
        int PVS(GameState& pos, int alpha, int beta, int depth, int ply, bool is_pv, SearchParams& params);
    private:
        void CheckStopConditions(SearchParams& params);
	}; // SearchInstance




    void IterativeDeepening(GameState& pos, SearchParams& params);
}