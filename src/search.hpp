#pragma once

#include "gamestate.hpp"
#include "movegen.hpp"
#include "engine_settings.hpp"
#include "evaluation.hpp"
#include "timer.hpp"
#include "ttable.hpp"


namespace anka {

    inline constexpr int num_killers_per_slot = 2;
    inline Move KillerMoves[EngineSettings::MAX_DEPTH][num_killers_per_slot];

    force_inline void SetKillerMove(Move m, int depth)
    {
        #ifdef ANKA_DEBUG
        if (KillerMoves[depth][0] != 0 && KillerMoves[depth][1] != 0)
            assert(KillerMoves[depth][0] != KillerMoves[depth][1]);
        #endif // ANKA_DEBUG

        if (KillerMoves[depth][0] == 0) {
            KillerMoves[depth][0] = m;
            return;
        }
        else if (KillerMoves[depth][0] == m) {
            return;
        }
        else {
            KillerMoves[depth][1] = m;
        }
    }


    struct SearchResult {
        u64 total_nodes = C64(0);
        long long total_time = 1;
        u64 nps = C64(0);
        int best_score = 0;
        int depth = 0;
        int pv_length = 0;
        Move* pv = nullptr;
        u64 fh = C64(1);
        u64 fh_f = C64(1);
    };

	class SearchInstance {
	public:
        u64 nodes_visited = C64(0);
        u64 pv_hits = C64(0);
        u64 num_fail_high = C64(1);
        u64 num_fail_high_first = C64(1);
        int selective_depth = C64(0);
        long long last_timecheck = 0;
	public:
        int Quiescence(GameState& pos, int alpha, int beta, SearchParams& params);
        int AlphaBeta(GameState& pos, int alpha, int beta, int depth, SearchParams& params);
        int PVS(GameState& pos, int alpha, int beta, int depth, SearchParams& params);
        int PVSRoot(GameState& pos, int alpha, int beta, int depth, SearchParams& params, Move &best_move);
    private:
        bool IsTimeUp(SearchParams& params);
	}; // SearchInstance




    void IterativeDeepening(GameState& pos, SearchParams& params);
}