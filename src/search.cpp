#include "search.hpp"
#include "util.hpp"
#include "evaluation.hpp"


namespace anka {
    static char side_char[2] = { 'w', 'b' };
    static Move principal_variation[MAX_PLY * MAX_PLY]{};
    
    static constexpr int nodes_per_time_check = 8191;
    static constexpr int R_null = 4;

    // returns true if there are no illegal moves in pv
    static bool ValidatePV(GameState& pos, const Move* pv)
    {
        MoveList<256> move_list;
        int moves_made = 0;
        bool success = true;
        while (Move move = pv[moves_made]) {
            move_list.GenerateLegalMoves(pos);
            if (move_list.Find(move)) {
                pos.MakeMove(move);
                moves_made++;
            }
            else {
                char move_str[6];
                move::ToString(move, move_str);
                fprintf(stderr, "Illegal move in pv: %s\n", move_str);
                success = false;
                break;
            }
        }

        while (moves_made--)
            pos.UndoMove();
        return success;
    }


    void SearchInstance::CheckTime(SearchParams& params)
    {
        auto curr_time = Timer::GetTimeInMs();
        auto time_passed = curr_time - last_timecheck;
        last_timecheck = curr_time;

        params.remaining_time-= time_passed;

            if (params.remaining_time < 0)
                params.uci_stop_flag = true;
    }

    void StartSearch(GameState& pos, SearchParams& params)
    {
        ClearKillerMoves();
        for (int i = 0; i < MAX_PLY * MAX_PLY; i++)
            principal_variation[i] = 0;
        

        bool skip_search = false;
        char best_move_str[6];
        Move best_move = 0;

        g_trans_table.IncrementAge();
        // check if there are no legal moves / only one legal move in the position
        {
            MoveList<256> move_list;
            move_list.GenerateLegalMoves(pos);

            if (move_list.length == 0) {
                skip_search = true;
            }
            else {
                best_move = move_list.moves[0].move;
                if (move_list.length == 1 || pos.IsDrawn()) {
                    skip_search = true;
                }
            }
        }

        if (!skip_search) {
            int max_depth = params.depth_limit > 0 ? params.depth_limit : MAX_PLY;

            // Iterative deepening loop
            SearchResult result;
            for (int d = 1; d <= max_depth; d++) {
                SearchInstance instance;
                instance.last_timecheck = params.start_time;

                int best_score = instance.PVS(pos, -ANKA_INFINITE, ANKA_INFINITE, d, 0, true, true, params);
                auto end_time = Timer::GetTimeInMs();

                if (params.uci_stop_flag) {
                    break;
                }

                best_move = principal_variation[0];
                result.best_score = best_score;
                result.depth = d;
                result.total_time = end_time - params.start_time + 1;
                result.total_nodes += instance.nodes_visited;
                result.nps = instance.nodes_visited / (result.total_time / 1000.0);
                result.pv = principal_variation;

                #ifdef STATS_ENABLED
                result.fh = instance.num_fail_high;
                result.fh_f = instance.num_fail_high_first;
                #endif


                result.Print(pos);
                ANKA_ASSERT(ValidatePV(pos, principal_variation));
            }

            // don't stop the search in infinite search mode unless a stop command is received
            while (params.infinite && !params.uci_stop_flag);
        }

        if (best_move > 0) {
            move::ToString(best_move, best_move_str);
        }
        else {
            best_move_str[0] = '0';
            best_move_str[1] = '0';
            best_move_str[2] = '0';
            best_move_str[3] = '0';
            best_move_str[4] = '\0';
        }
        printf("bestmove %s\n", best_move_str);
        STATS(g_trans_table.PrintStatistics());

        params.is_searching = false;
    }

    int SearchInstance::Quiescence(GameState& pos, int alpha, int beta, int ply, SearchParams& params)
    {
        ANKA_ASSERT(beta > alpha);
        if (params.check_timeup && (nodes_visited & nodes_per_time_check) == 0) {
            CheckTime(params);
        }

        if (pos.IsDrawn())
            return 0;

        MoveList<256> move_list;
        bool in_check = pos.InCheck();

        if (in_check) {
            move_list.GenerateLegalMoves(pos); // all check evasion moves
            if (move_list.length == 0) {
                return -ANKA_MATE + ply;
            }
        }
        else {
            move_list.GenerateLegalCaptures(pos);

            // stand pat
            int eval = pos.ClassicalEvaluation();
            if (eval > alpha) {
                if (eval >= beta) {
                    return eval;
                }

                alpha = eval;
            }
        }

        while (move_list.length > 0) {
            Move move = move_list.PopBest();
            nodes_visited++;
            pos.MakeMove(move);
            int score = -Quiescence(pos, -beta, -alpha, ply + 1, params);
            pos.UndoMove();

            if (params.uci_stop_flag)
                return alpha;

            if (score > alpha) {
                if (score >= beta) { // beta cutoff
                    return score;
                }

                alpha = score;
            }
        }

        return alpha;
    }

    template<bool pruning>
    int SearchInstance::PVS(GameState& pos, int alpha, int beta, int depth, int ply, bool is_pv, bool null_pruning, SearchParams& params)
    {
        ANKA_ASSERT(beta > alpha);
        if (params.check_timeup && (nodes_visited & nodes_per_time_check) == 0) {
            CheckTime(params);
        }

        int old_alpha = alpha;
        static_assert(MAX_PLY == 128, "pv_index calculation must be modified for max_ply != 128");
        int pv_index = 0;
        int pv_index_next = 0;

        if (is_pv) {
            pv_index = ply << 7;
            pv_index_next = pv_index + 128;
            principal_variation[pv_index] = 0;
        }


        if (pos.IsDrawn()) {
            return 0;
        }

        if (depth <= 0 || ply >= MAX_PLY)
            return Quiescence(pos, alpha, beta, ply, params);

        u64 pos_key = pos.PositionKey();
        TTRecord probe_result;
        Move hash_move = 0;
        
        if (g_trans_table.Get(pos_key, probe_result, ply)) {
            hash_move = probe_result.move;
            auto node_type = probe_result.GetNodeType();
            if (!is_pv && probe_result.depth >= depth) {
                switch (node_type) {
                case NodeType::EXACT: {
                    return probe_result.value;
                }
                case NodeType::UPPERBOUND: {
                    if (alpha > probe_result.value)
                        return probe_result.value;
                    break;
                }
                case NodeType::LOWERBOUND: {
                    if (probe_result.value >= beta)
                        return probe_result.value;
                    break;
                }
                } // end of switch
            }
        }

        int eval = pos.ClassicalEvaluation();

        MoveList<256> list;
        bool in_check = list.GenerateLegalMoves(pos);

        if (list.length == 0) {
            if (in_check) {
                return -ANKA_MATE + ply;
            }
            else {
                return 0; // stalemate
            }
        }

        if constexpr (pruning) {
            if (!in_check && !is_pv) {
                if (pos.AllyNonPawnPieces() > 0) {
                    // Static Null Move Pruning (Reverse Futility)
                    int margin = 75 * depth;
                    if (depth < 7 && eval - beta >= margin) {
                        return eval;
                    }

                    // Null move reductions
                    if (null_pruning && eval >= beta) {
                        pos.MakeNullMove();
                        int score = -PVS(pos, -beta, -beta + 1, depth - R_null, ply + 1, false, false, params);
                        pos.UndoNullMove();

                        if (score >= beta) {
                            int R = depth > 6 ? 4 : 3;
                            depth -= R;
                            if (depth <= 0)
                                return Quiescence(pos, alpha, beta, ply, params);
                        }
                    }
                }
            }
        }


        // First move
        Move best_move = list.PopBest(hash_move, KillerMoves[ply][0], KillerMoves[ply][1]);
        nodes_visited++;
        pos.MakeMove(best_move);
        int best_score = -PVS(pos, -beta, -alpha, depth - 1, ply + 1, is_pv, true, params);
        pos.UndoMove();

        if (best_score > alpha) {
            if (is_pv) {
                principal_variation[pv_index] = best_move;
                memcpy(principal_variation + pv_index + 1, principal_variation + pv_index_next, (MAX_PLY - ply - 1) * sizeof(Move));
            }
            if (best_score >= beta) {
                STATS(num_fail_high++);
                STATS(num_fail_high_first++);
                if (!in_check && move::IsQuiet(best_move))
                    SetKillerMove(best_move, ply);
                g_trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, best_move, best_score, ply, params.uci_stop_flag);
                return best_score;
            }
            ANKA_ASSERT(is_pv);
            alpha = best_score; // not reached in zero-window
        }

        if (params.uci_stop_flag)
            return beta;

        // Rest of the moves
        while (list.length > 0) {
            Move move = list.PopBest();
            nodes_visited++;
            pos.MakeMove(move);
            int score = -PVS(pos, -alpha - 1, -alpha, depth - 1, ply + 1, false, true, params); // zero window
            if (score > alpha && score < beta) { // always false in zero-window
                score = -PVS(pos, -beta, -alpha, depth - 1, ply + 1, true, true, params);
                if (score > alpha) {
                    ANKA_ASSERT(is_pv);
                    alpha = score;
                    principal_variation[pv_index] = move;
                    memcpy(principal_variation + pv_index + 1, principal_variation + pv_index_next, (MAX_PLY - ply - 1) * sizeof(Move));
                }
            }
            pos.UndoMove();
            if (score > best_score) {
                if (score >= beta) {
                    STATS(num_fail_high++);
                    g_trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, move, score, ply, params.uci_stop_flag);
                    if (!in_check && move::IsQuiet(move))
                        SetKillerMove(move, ply);
                    return score;
                }
                best_score = score;
                best_move = move;
            }
            if (params.uci_stop_flag)
                return best_score;
        }


        if (best_score > old_alpha) {
            ANKA_ASSERT(is_pv);
            g_trans_table.Put(pos_key, NodeType::EXACT, depth, best_move, best_score, ply, params.uci_stop_flag);
        }
        else {
            g_trans_table.Put(pos_key, NodeType::UPPERBOUND, depth, best_move, best_score, ply, params.uci_stop_flag);
        }

        return best_score;
    }
}