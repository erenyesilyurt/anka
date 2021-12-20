#include "search.hpp"
#include "util.hpp"
#include "evaluation.hpp"
#include "tbprobe.h"

namespace anka {
    
    

    namespace {
        constexpr int PV_NODE = 1;
        constexpr int NOT_PV= 0;
        constexpr int nodes_per_time_check = 8191;
        constexpr int MAX_PV_LENGTH = MAX_DEPTH + 1;

        Move principal_variation[MAX_PV_LENGTH]{};
        int LMR[MAX_DEPTH+1][256]{};
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

        constexpr int TB_WIN_SCORE = 10000;

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

            if (params.remaining_time <= 0)
                params.uci_stop_flag = true;
    }

    void StartSearch(GameState& pos, SearchParams& params)
    {
        killers.Clear();
        for (int i = 0; i < MAX_PV_LENGTH; i++) {
            principal_variation[i] = move::NO_MOVE;
        }

        bool skip_search = false;
        char best_move_str[6];
        Move best_move = move::NO_MOVE;

        
        g_trans_table.IncrementAge();

        
        MoveList<256> root_moves;
        bool in_check = root_moves.GenerateLegalMoves(pos);

        if (root_moves.length == 0) {
            if (in_check)
                printf("info depth 0 score mate 0\n");
            else
                printf("info depth 0 score cp 0\n");
            skip_search = true;
        }
        else {
            best_move = root_moves.moves[0].move;
            if (root_moves.length == 1) {
                skip_search = true;
            }
        }
        
        // Probe tablebase
        if (!skip_search && pos.PieceCount() <= TB_LARGEST) {
            auto tb_result = tb_probe_root(pos.WhitePieces(), pos.BlackPieces(), pos.Kings(),
                pos.Queens(), pos.Rooks(), pos.Bishops(), pos.Knights(), pos.Pawns(),
                pos.HalfMoveClock(), pos.EnPassantSquare(), !pos.SideToPlay(), NULL);

            if (tb_result != TB_RESULT_CHECKMATE &&
                tb_result != TB_RESULT_STALEMATE &&
                tb_result != TB_RESULT_FAILED)
            {
                auto wdl = TB_GET_WDL(tb_result);
                auto from = TB_GET_FROM(tb_result);
                auto to = TB_GET_TO(tb_result);
                auto prom = TB_GET_PROMOTES(tb_result);

                PieceType prom_piece = NO_PIECE;
                if (prom == 1) {
                    prom_piece = QUEEN;
                }
                else if (prom == 2) {
                    prom_piece = ROOK;
                }
                else if (prom == 3) {
                    prom_piece = BISHOP;
                }
                else if (prom == 4) {
                    prom_piece = KNIGHT;
                }

                Move tb_move = root_moves.FindTBMove(from, to, prom_piece);
                if (tb_move) {

                    if (wdl == TB_WIN) {
                        printf("info depth 0 score cp %d\n", TB_WIN_SCORE);
                    }
                    else if (wdl == TB_LOSS) {
                        printf("info depth 0 score cp %d\n", -TB_WIN_SCORE);
                    }
                    else {
                        printf("info depth 0 score cp 0\n");
                    }

                    best_move = tb_move;
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
            result.total_time = 1;
            result.pv = principal_variation;
            for (int d = 1; d <= max_depth; d++) {          
                SearchInstance instance;
                instance.last_timecheck = Timer::GetTimeInMs();

                auto iter_start_time = instance.last_timecheck;
                int best_score = instance.PVS<PV_NODE, true>(pos, -ANKA_INFINITE, ANKA_INFINITE, d, params);
                auto iter_end_time = Timer::GetTimeInMs();
                auto delta_time = iter_end_time - iter_start_time;


                if (params.uci_stop_flag) {
                    break;
                }

                best_move = instance.root_best_move;
                int pv_length = g_trans_table.ExtractPV(pos, best_move, principal_variation, MAX_PV_LENGTH);
                

                result.best_score = best_score;
                result.depth = d;
                result.tb_hits += instance.tb_hits;
                result.total_time += delta_time;
                result.total_nodes += instance.nodes_visited;
                result.nps = result.total_nodes / (result.total_time / 1000.0);

                #ifdef STATS_ENABLED
                result.fh = instance.num_fail_high;
                result.fh_f = instance.num_fail_high_first;
                #endif


                result.Print(pos, pv_length);
                
                if (params.check_timeup) {
                    if (delta_time > params.remaining_time * 5 >> 3) {
                        break;
                    }
                }
            }
        }

        // don't stop the search in infinite search mode unless a stop command is received
        while (params.infinite && !params.uci_stop_flag);

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

    template<bool is_pv, bool is_root>
    int SearchInstance::PVS(GameState& pos, int alpha, int beta, int depth, SearchParams& params)
    {
        ANKA_ASSERT(beta > alpha);
        int ply = pos.Ply();
        int old_alpha = alpha;

        if constexpr (!is_root) {
            if (params.check_timeup && (nodes_visited & nodes_per_time_check) == 0) {
                CheckTime(params);
            }

            if (pos.IsDrawn()) {
                return 0;
            }

            if (depth <= 0 || depth > MAX_DEPTH)
                return Quiescence(pos, alpha, beta, params);
        }

        u64 pos_key = pos.PositionKey();
        TTRecord probe_result;
        Move hash_move = 0;
        int hash_eval = 0;
        NodeType hash_node_type = NodeType::NONE;
        if (g_trans_table.Get(pos_key, probe_result, ply)) {
            hash_move = probe_result.move;
            if constexpr (!is_pv) {
                hash_eval = probe_result.value;
                hash_node_type = probe_result.GetNodeType();
                if (probe_result.depth >= depth) {
                    switch (hash_node_type) {
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
        }      

        bool in_check = s_stack->move_list[ply].GenerateLegalMoves(pos);
        if constexpr (!is_root) {
            if (s_stack->move_list[ply].length == 0) {
                if (in_check) {
                    return -ANKA_MATE + ply;
                }
                else {
                    return 0; // stalemate
                }
            }
        
            if (pos.PieceCount() <= TB_LARGEST && pos.HalfMoveClock() == 0) {
                auto wdl = tb_probe_wdl(pos.WhitePieces(), pos.BlackPieces(), pos.Kings(), pos.Queens(),
                    pos.Rooks(), pos.Bishops(), pos.Knights(), pos.Pawns(),
                    pos.EnPassantSquare(), !pos.SideToPlay());
                if (wdl != TB_RESULT_FAILED) {
                    tb_hits++;
                    if (wdl == TB_WIN) {
                        return TB_WIN_SCORE - ply;
                    }
                    else if (wdl == TB_LOSS) {
                        return -TB_WIN_SCORE + ply;
                    }
                    else {
                        return 0;
                    }
                }
            }
        }


        if constexpr (!is_pv) {
            if (!in_check) {
                if (pos.AllyNonPawnPieces() > 0) {
                    int eval = 0;
                    if (hash_node_type == NodeType::EXACT) {
                        eval = hash_eval;
                    }
                    else if (hash_node_type == NodeType::LOWERBOUND) {
                        eval = Max(pos.ClassicalEvaluation(), hash_eval);
                    }
                    else if (hash_node_type == NodeType::UPPERBOUND) {
                        eval = Min(pos.ClassicalEvaluation(), hash_eval);
                    }
                    else {
                        eval = pos.ClassicalEvaluation();
                    }

                    auto eval_margin = eval - beta;
                    // Static Null Move Pruning (Reverse Futility)
                    if (depth < 7 && eval_margin >= 75 * depth) {
                        return eval;
                    }

                    // Null move reductions
                    if (pos.LastMove() != move::NULL_MOVE && eval_margin >= 0 && nmp_enabled) {
                        int R_null = 4 + Min(3, eval_margin / 150) + depth / 4; // adaptive reduction, like stockfish
                        pos.MakeNullMove();
                        int score = -PVS<NOT_PV>(pos, -beta, -beta + 1, depth - R_null, params);
                        pos.UndoNullMove();

                        if (score >= beta) {
                            if (depth < 12)
                                return score;

                            // verification at high depths
                            nmp_enabled = false;
                            int v_score = PVS<NOT_PV>(pos, -beta, -beta+1, depth - R_null, params);
                            nmp_enabled = true;
                            if (v_score >= beta)
                                return v_score;
                        }
                    }
                }
            }
        }
        else {
            // internal iterative deepening
            if (hash_move == move::NO_MOVE && depth > 7) {
                PVS<PV_NODE>(pos, alpha, beta, depth >> 1, params);
                if (g_trans_table.Get(pos_key, probe_result, ply)) {
                    hash_move = probe_result.move;
                }
            }
        }

        s_stack->move_list[ply].GenerateLegalMoves(pos);
        int moves_made = 0;
        Move best_move = move::NO_MOVE;
        int best_score = -ANKA_INFINITE;
        while (s_stack->move_list[ply].length > 0) {
            Move move = move::NO_MOVE;
            int score = -ANKA_INFINITE;

            if (moves_made == 0) {
                move = s_stack->move_list[ply].PopBest(hash_move,
                    killers.moves[ply][0], killers.moves[ply][1]);
                pos.MakeMove(move);
                score = -PVS<is_pv>(pos, -beta, -alpha, depth - 1, params);
            }
            else {
                move = s_stack->move_list[ply].PopBest();
                pos.MakeMove(move);

                int reduction = 0;
                if (!in_check && depth >= 3 && move::IsQuiet(move)) {
                    reduction = LMR[depth][moves_made];
                    if constexpr (is_pv)
                        reduction -= 1;

                    reduction = Clamp(reduction, 0, depth - 2);
                }
               
                score = -PVS<NOT_PV>(pos, -alpha - 1, -alpha, depth-1-reduction, params);
                if (score > alpha && score < beta) // always false in zero-window
                    score = -PVS<PV_NODE>(pos, -beta, -alpha, depth - 1, params);
            }
            pos.UndoMove();
            moves_made++;
            nodes_visited++;

            if (score > best_score) {
                best_score = score;
                best_move = move;
            }

            if (score > alpha) {
                if (score >= beta) {
                    STATS(num_fail_high++); STATS(num_fail_high_first++);    
                    if (!in_check && move::IsQuiet(move)) {
                        killers.Put(move, ply);
                    }
                    g_trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, move, score, ply, params.uci_stop_flag);
                    return score;
                }
                ANKA_ASSERT(is_pv);
                alpha = score;
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

        if constexpr (is_root) {
            root_best_move = best_move;
        }
        return best_score;
    }
}