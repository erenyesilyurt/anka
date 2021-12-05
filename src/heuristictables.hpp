#pragma once
#include "move.hpp"
#include "engine_settings.hpp"

namespace anka
{
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