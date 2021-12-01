#pragma once
#include "move.hpp"
#include "engine_settings.hpp"

namespace anka
{
    struct HistoryTable
    {
        int history[NUM_SIDES][64][64]{};

        void Clear()
        {
            for (Side color : {WHITE, BLACK}) {
                for (Square fr = 0; fr < 64; fr++) {
                    for (Square to = 0; to < 64; to++) {
                        history[color][fr][to] = 0;
                    }
                }
            }
        }

        force_inline void Update(Side color, Square from, Square to, int depth)
        {
            history[color][from][to] += depth * depth;
            if (history[color][from][to] > 20'000) {
                history[color][from][to] /= 2;
            }
        }
    };

    struct KillersTable
    {
        Move moves[MAX_PLY][2]{};

        force_inline void Put(Move m, int ply)
        {
            #ifdef ANKA_DEBUG
            if (moves[ply][0] != 0 && moves[ply][1] != 0)
                assert(moves[ply][0] != moves[ply][1]);
            #endif // ANKA_DEBUG

            if (moves[ply][0] != m) {
                moves[ply][1] = moves[ply][0];
                moves[ply][0] = m;
            }
        }

        force_inline void Clear()
        {
            for (int i = 0; i < MAX_PLY; i++) {
                moves[i][0] = move::NO_MOVE;
                moves[i][1] = move::NO_MOVE;
            }
        }
    };





}