#include "search.hpp"
#include "util.hpp"
#include "evaluation.hpp"


namespace anka {
    
    

    namespace {
        constexpr int nodes_per_time_check = 8191;
        constexpr int R_null = 4;

        int LMR[MAX_DEPTH+1][256]{};
        HistoryTable history_table;
        KillersTable killers;
        SearchStack* s_stack;

        void InitLMR()
        {
            for (int depth = 1; depth <= MAX_DEPTH; depth++) {
                for (int m_count = 1; m_count < 256; m_count++) {
                    LMR[depth][m_count] = 0.5 + log(depth) * log(m_count) * 0.5;
                }
            }
        }

        // returns true if there are no illegal moves in pv
        bool ValidatePV(GameState& pos, const Move* pv)
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
    }

    bool InitSearch()
    {
        InitLMR();
        s_stack = (SearchStack*)malloc(sizeof(SearchStack));
        if (!s_stack) {
            fprintf(stderr, "Failed to allocate search stack memory\n");
            return false;
        }
        return true;
    }

    void FreeSearchStack()
    {
        free(s_stack);
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
        killers.Clear();
        for (int i = 0; i < (MAX_DEPTH+1) * (MAX_DEPTH+1); i++)
            s_stack->pv[i] = move::NO_MOVE;
        

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
            int max_depth = MAX_DEPTH;
            if (params.depth_limit > 0 && params.depth_limit < MAX_DEPTH) {
                max_depth = params.depth_limit;
            }

            // Iterative deepening loop
            SearchResult result;         
            for (int d = 1; d <= max_depth; d++) {          
                SearchInstance instance;
                instance.last_timecheck = Timer::GetTimeInMs();

                int best_score = instance.PVS(pos, -ANKA_INFINITE, ANKA_INFINITE, d, true, params);
                auto end_time = Timer::GetTimeInMs();

                if (params.uci_stop_flag) {
                    break;
                }

                best_move = s_stack->pv[0];
                result.best_score = best_score;
                result.depth = d;
                result.total_time = end_time - params.start_time + 1;
                result.total_nodes += instance.nodes_visited;
                result.nps = result.total_nodes / (result.total_time / 1000.0);
                result.pv = s_stack->pv;

                #ifdef STATS_ENABLED
                result.fh = instance.num_fail_high;
                result.fh_f = instance.num_fail_high_first;
                #endif


                result.Print(pos);

                ANKA_ASSERT(ValidatePV(pos, s_stack->pv));
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

    int SearchInstance::Quiescence(GameState& pos, int alpha, int beta, SearchParams& params)
    {
        ANKA_ASSERT(beta > alpha);
        int ply = pos.Ply();
        if (params.check_timeup && (nodes_visited & nodes_per_time_check) == 0) {
            CheckTime(params);
        }

        if (pos.IsDrawn())
            return 0;

        if (ply > MAX_PLY)
            return pos.ClassicalEvaluation();


        if (pos.InCheck()) {
            s_stack->move_list[ply].GenerateLegalMoves(pos); // all check evasion moves
            if (s_stack->move_list[ply].length == 0) {
                return -ANKA_MATE + ply;
            }
        }
        else {
            s_stack->move_list[ply].GenerateLegalCaptures(pos);

            // stand pat
            int eval = pos.ClassicalEvaluation();
            if (eval > alpha) {
                if (eval >= beta) {
                    return eval;
                }

                alpha = eval;
            }
        }

        while (s_stack->move_list[ply].length > 0) {
            Move move = s_stack->move_list[ply].PopBest();
            nodes_visited++;
            pos.MakeMove(move);
            int score = -Quiescence(pos, -beta, -alpha, params);
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
    int SearchInstance::PVS(GameState& pos, int alpha, int beta, int depth, bool is_pv, SearchParams& params)
    {
        ANKA_ASSERT(beta > alpha);
        int ply = pos.Ply();


        if (params.check_timeup && (nodes_visited & nodes_per_time_check) == 0) {
            CheckTime(params);
        }
        
        int old_alpha = alpha;
        int pv_index = 0;
        int pv_index_next = 0;

        if (is_pv) {
            pv_index = ply * MAX_DEPTH;
            pv_index_next = pv_index + MAX_DEPTH;
            s_stack->pv[pv_index] = 0;
        }


        if (pos.IsDrawn()) {
            return 0;
        }

        if (depth <= 0)
            return Quiescence(pos, alpha, beta, params);

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

        bool in_check = s_stack->move_list[ply].GenerateLegalMoves(pos);
        if (s_stack->move_list[ply].length == 0) {
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
                    if (pos.LastMove() != move::NULL_MOVE && eval >= beta) {
                        pos.MakeNullMove();
                        int score = -PVS(pos, -beta, -beta + 1, depth - R_null,  false, params);
                        pos.UndoNullMove();
                        if (score >= beta) {
                            int R = depth > 6 ? 4 : 3;
                            depth -= R;
                            if (depth <= 0)
                                return Quiescence(pos, alpha, beta, params);
                        }
                    }
                }
            }
        }


        int moves_made = 0;

        Side color = pos.SideToPlay();
        // First move
        Move best_move = s_stack->move_list[ply].PopBest(hash_move,
            killers.moves[ply][0], killers.moves[ply][1],
            color, history_table);
        pos.MakeMove(best_move);
        int best_score = -PVS(pos, -beta, -alpha, depth - 1, is_pv, params);
        pos.UndoMove();
        nodes_visited++;
        moves_made++;
        

        if (best_score > alpha) {
            if (is_pv) {
                s_stack->pv[pv_index] = best_move;
                memcpy(s_stack->pv + pv_index + 1, s_stack->pv + pv_index_next, (MAX_DEPTH - ply - 1) * sizeof(Move));
            }
            if (best_score >= beta) {
                STATS(num_fail_high++);
                STATS(num_fail_high_first++);
                if (!in_check && move::IsQuiet(best_move)) {
                    history_table.Update(pos.SideToPlay(), move::FromSquare(best_move), move::ToSquare(best_move), depth);
                    killers.Put(best_move, ply);
                }
                g_trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, best_move, best_score, ply, params.uci_stop_flag);
                return best_score;
            }
            ANKA_ASSERT(is_pv);
            alpha = best_score; // not reached in zero-window
        }

        if (params.uci_stop_flag)
            return beta;

        // Rest of the moves
        while (s_stack->move_list[ply].length > 0) {
            Move move = s_stack->move_list[ply].PopBest();
            pos.MakeMove(move);
            
            int reduction = 0;
            if (depth >= 3 && move::IsQuiet(move)) {
                reduction = LMR[depth][moves_made];
                int history_score = history_table.history[pos.SideToPlay()][move::FromSquare(move)][move::ToSquare(move)];
                reduction -= history_score / 10'000;
                if (is_pv)
                    reduction -= 1;

                reduction = Clamp(reduction, 0, depth - 2);
            }

            int score = -PVS(pos, -alpha - 1, -alpha, depth-1-reduction, false, params); // zero window
            if (score > alpha && score < beta) { // always false in zero-window
                score = -PVS(pos, -beta, -alpha, depth - 1, true, params);
                if (score > alpha) {
                    ANKA_ASSERT(is_pv);
                    alpha = score;
                    s_stack->pv[pv_index] = move;
                    memcpy(s_stack->pv + pv_index + 1, s_stack->pv + pv_index_next, (MAX_DEPTH - ply - 1) * sizeof(Move));
                }
            }
            pos.UndoMove();
            nodes_visited++;
            moves_made++;
            if (score > best_score) {
                if (score >= beta) {
                    STATS(num_fail_high++);
                    g_trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, move, score, ply, params.uci_stop_flag);
                    if (!in_check && move::IsQuiet(move)) {
                        history_table.Update(pos.SideToPlay(), move::FromSquare(move), move::ToSquare(move), depth);
                        killers.Put(move, ply);
                    }
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