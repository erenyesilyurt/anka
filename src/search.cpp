#include "search.hpp"
#include "io.hpp"
#include "util.hpp"
#include "evaluation.hpp"


namespace anka {
    static char side_char[2] = { 'w', 'b' };
    static Move principal_variation[EngineSettings::PV_BUFFER_LENGTH];

    //  see "Time management procedure in computer chess , Vladan Vuckovic, Rade Solak "
    static long long AllocateTime(const GameState& pos, SearchParams& params)
    {
        long long available_time = 0;
        long long increment = 0;
        if (pos.SideToPlay() == side::WHITE) {
            available_time = params.wtime;
            increment = params.winc;
        }
        else {
            available_time = params.btime;
            increment = params.binc;
        }

        available_time -= EngineSettings::MOVE_OVERHEAD;

        int total_material = pos.TotalMaterial();

        // guess the number of remaining plies based on statistical data
        int num_remaining = 0;
        if (total_material < 1800) {
            num_remaining = (total_material >> 6) + 3; // approx. 0.013*total_material + 5.50 for (0, 1800)
        }
        else if (total_material < 6100) {
            num_remaining = (total_material >> 8) + 20; // approx. 0.003*x + 23.2 for (1800, 6100)
        }
        else {
            num_remaining = (total_material >> 6) - 57; // approx 0.013*x - 39.65 for (>6100)
        }

        // increment
        available_time += increment;

        return (available_time / num_remaining);
    }

    // fail-soft PVS
    int SearchInstance::PVS(GameState& pos, int alpha, int beta, int depth, SearchParams& params)
    {
        ANKA_ASSERT(beta > alpha);

        int old_alpha = alpha;

        if ((nodes_visited & 4095) == 0) {
            CheckStopConditions(params);
        }

        if (pos.HalfMoveClock() >= 100 || pos.IsRepetition()) {
            return 0;
        }


        u64 pos_key = pos.PositionKey();
        TTResult probe_result;
        Move hash_move = 0;
        if (trans_table.Get(pos_key, probe_result)) {
            hash_move = probe_result.move;
            if (probe_result.depth >= depth) {
                switch (probe_result.type) {
                    case anka::NodeType::PV: {
                        return probe_result.score;
                    }
                    case anka::NodeType::UPPERBOUND: {
                        beta = Min(beta, probe_result.score);
                        break;
                    }
                    case anka::NodeType::LOWERBOUND: {
                        alpha = Max(alpha, probe_result.score);
                        break;
                    }
                }

                if (alpha >= beta) // beta cutoff
                    return alpha;
            }
        }

        if (depth == 0)
            return Quiescence(pos, alpha, beta, params);


        MoveList<256> list;
        bool in_check = list.GenerateLegalMoves(pos);

        if (list.length == 0) {
            if (in_check) {
                //io::IndentedPrint(pos.SearchDepth(), "AB result terminal %d\n", Clamp(-ANKA_INFINITE, alpha, beta));
                return -ANKA_INFINITE;
            }
            else {
                //io::IndentedPrint(pos.SearchDepth(), "AB result terminal %d\n", Clamp(0, alpha, beta));
                return 0;
            }
        }


        int killer_move_1 = KillerMoves[depth][0];
        int killer_move_2 = KillerMoves[depth][1];
        for (int j = 0; j < list.length; j++) {
            if (list.moves[j].move == hash_move) {
                // TODO: bring hash move to list index 0
                list.moves[j].score = move::PV_SCORE;
            }
            else if ((list.moves[j].move == killer_move_1) || (list.moves[j].move == killer_move_2)) {
                list.moves[j].score = move::KILLER_SCORE;
            }
        }


        // First move
        Move best_move = list.PopBest();
        nodes_visited++;
        char move_str[6];
        move::ToString(best_move, move_str);
        //io::IndentedPrint(pos.SearchDepth(), "AB %c firstmove %s alpha %d beta %d\n", side_char[pos.SideToPlay()], move_str, alpha, beta);
        pos.MakeMove(best_move);
        int best_score = -PVS(pos, -beta, -alpha, depth - 1, params);
        pos.UndoMove();

        if (best_score > alpha) {
            if (best_score >= beta) {
                trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, best_move, best_score);
                if (!in_check && move::IsQuiet(best_move))
                    SetKillerMove(best_move, depth);
                return best_score;
            }
            else {
                alpha = best_score;
            }
        }

        if (params.uci_stop_flag)
            return best_score;

        // Rest of the moves
        while (list.length > 0) {
            Move move = list.PopBest();
            nodes_visited++;

            //move::ToString(move, move_str);
            //io::IndentedPrint(pos.SearchDepth(), "AB %c move %s alpha %d beta %d\n", side_char[pos.SideToPlay()], move_str, alpha, beta);
            pos.MakeMove(move);
            int score = -PVS(pos, -alpha - 1, -alpha, depth - 1, params); // zero window
            if (score > alpha && score < beta) {
                score = -PVS(pos, -beta, -alpha, depth - 1, params);
                if (score > alpha)
                    alpha = score;
            }
            pos.UndoMove();
            if (score > best_score) {
                if (score >= beta) {
                    trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, move, score);
                    if (!in_check && move::IsQuiet(move))
                        SetKillerMove(move, depth);
                    return score;
                }
                best_score = score;
                best_move = move;
            }
            if (params.uci_stop_flag)
                return best_score;
        }

        
        if (best_score > old_alpha) {
            ANKA_ASSERT(best_score < beta);
            trans_table.Put(pos_key, NodeType::PV, depth, best_move, best_score);
        }
        else {
            trans_table.Put(pos_key, NodeType::UPPERBOUND, depth, best_move, best_score);
        }

        return best_score;
    }


    void IterativeDeepening(GameState& pos, SearchParams& params)
    {
        params.start_time = Timer::GetTimeInMs();

        bool skip_search = false;
        char best_move_str[6];

        trans_table.IncrementAge();
        // check if there is only one legal move in position
        {
            MoveList<256> move_list;
            move_list.GenerateLegalMoves(pos);
            if (move_list.length == 0) {
                principal_variation[0] = 0;
            }
            else {
                principal_variation[0] = move_list.moves[0].move;
                if (move_list.length == 1) {
                    skip_search = true;
                }
            }    
        }


        if (!skip_search) {

            // start with an initial search regardless of time controls so we always have something in pv
            SearchInstance initial_search;
            constexpr int initial_search_depth = 3;
            initial_search.AlphaBeta(pos, -ANKA_INFINITE, ANKA_INFINITE, initial_search_depth, params);
            TTResult initial_probe{};
            trans_table.Get(pos.PositionKey(), initial_probe);
            principal_variation[0] = initial_probe.move;

            // configure time controls
            int max_depth = EngineSettings::MAX_DEPTH;
            if (!params.infinite) {
                if (params.wtime || params.btime) {
                    params.check_timeup = true;
                    params.movetime = AllocateTime(pos, params);
                }
                else if (params.movetime) {
                    params.check_timeup = true;
                }
                else if (params.search_depth > 0) {
                    max_depth = params.search_depth;
                }
            }
            
            // Iterative deepening loop
            SearchResult result;
            for (int d = 1; d <= max_depth; d++) {
                SearchInstance instance;
                instance.last_timecheck = params.start_time;

                int best_score = instance.PVS(pos, -ANKA_INFINITE, ANKA_INFINITE, d, params);
                auto end_time = Timer::GetTimeInMs();

                if (params.uci_stop_flag) {
                    result.total_time = end_time - params.start_time + 1;
                    break;
                }

                result.best_score = best_score;
                result.fh = instance.num_fail_high;
                result.fh_f = instance.num_fail_high_first;
                result.depth = d;
                result.total_time = end_time - params.start_time + 1;
                result.total_nodes += instance.nodes_visited;
                result.nps = instance.nodes_visited / (result.total_time / 1000.0);
                result.pv_length = trans_table.GetPrincipalVariation(pos, principal_variation, EngineSettings::PV_BUFFER_LENGTH);
                //ANKA_ASSERT(result.pv_length > 0);
                result.pv = principal_variation;


                io::PrintSearchResults(pos, result);
            }
        }

        if (principal_variation[0] > 0) {
            move::ToString(principal_variation[0], best_move_str);
        }
        else {
            best_move_str[0] = '0';
            best_move_str[1] = '0';
            best_move_str[2] = '0';
            best_move_str[3] = '0';
            best_move_str[4] = '\0';
        }
        printf("bestmove %s\n", best_move_str);
    }



    // Fail-soft
    int SearchInstance::Quiescence(GameState& pos, int alpha, int beta, SearchParams &params) 
    {

        //io::IndentedPrint(pos.SearchDepth(), "QS %c alpha %d beta %d\n", side_char[pos.SideToPlay()], alpha, beta);

        ANKA_ASSERT(beta > alpha);
        if ((nodes_visited & 4095) == 0) {
            CheckStopConditions(params);
        }
        
     
        if (pos.HalfMoveClock() >= 100 || pos.IsRepetition()) {
            //io::IndentedPrint(pos.SearchDepth(), "QS %c result terminal %d\n", side_char[pos.SideToPlay()], Clamp(0, alpha, beta));
            return 0;
        }

        MoveList<256> list;
        bool in_check = pos.InCheck();
        
        if (in_check) {
            list.GenerateLegalMoves(pos); // all check evasion moves
            if (list.length == 0) {
                return -ANKA_INFINITE;
            }
        }
        else {
            list.GenerateLegalCaptures(pos);
            // stand pat
            int eval = ClassicEvaluation(pos);

            if (eval >= beta) {
                //io::IndentedPrint(pos.SearchDepth(), "QS %c result standpat betacutoff %d (%d >= %d)\n", side_char[pos.SideToPlay()], beta, eval, beta);
                return eval;
            }

            if (eval > alpha)
                alpha = eval;
        }


        while (list.length > 0) {
            Move move = list.PopBest();
            char move_str[6];
            move::ToString(move, move_str);
            //io::IndentedPrint(pos.SearchDepth(), "QS %c move %s alpha %d beta %d\n", side_char[pos.SideToPlay()], move_str, alpha, beta);
            nodes_visited++;
            pos.MakeMove(move);
            int score = -Quiescence(pos, -beta, -alpha, params);
            pos.UndoMove();

            if (params.uci_stop_flag)
                return alpha;

            if (score > alpha) {
                if (score >= beta) { // beta cutoff
                    //io::IndentedPrint(pos.SearchDepth(), "QS %c result betacutoff %d (%d >= %d)\n", side_char[pos.SideToPlay()], beta, score, beta);
                    return score;
                }

                alpha = score;
            }
        }

        //io::IndentedPrint(pos.SearchDepth(), "QS %c result alpha %d\n", side_char[pos.SideToPlay()], alpha);
        return alpha;
    }

    int SearchInstance::AlphaBeta(GameState& pos, int alpha, int beta, int depth, SearchParams& params)
    {
        ////io::IndentedPrint(pos.SearchDepth(), "AB %c alpha %d beta %d\n", side_char[pos.SideToPlay()], alpha, beta);
        ANKA_ASSERT(beta > alpha);

        int old_alpha = alpha;

        if ((nodes_visited & 4095) == 0) {
            CheckStopConditions(params);
        }

        if (pos.HalfMoveClock() >= 100 || pos.IsRepetition()) {
            //io::IndentedPrint(pos.SearchDepth(), "AB result terminal %d\n", Clamp(0, alpha, beta));
            return 0;
        }


        u64 pos_key = pos.PositionKey();
        TTResult probe_result;
        Move hash_move = 0;
        if (trans_table.Get(pos_key, probe_result)) {
            hash_move = probe_result.move;
            if (probe_result.depth >= depth) {
                switch (probe_result.type) {
                case anka::NodeType::PV: {
                    return probe_result.score;
                }
                case anka::NodeType::UPPERBOUND: {
                    beta = Min(beta, probe_result.score);
                    break;
                }
                case anka::NodeType::LOWERBOUND: {
                    alpha = Max(alpha, probe_result.score);
                    break;
                }
                }

                if (alpha >= beta) // beta cutoff
                    return alpha;
            }
        }

        if (depth == 0)
            return Quiescence(pos, alpha, beta, params);


        MoveList<256> list;
        bool in_check = list.GenerateLegalMoves(pos);

        if (list.length == 0) {
            if (in_check) {
                //io::IndentedPrint(pos.SearchDepth(), "AB result terminal %d\n", Clamp(-ANKA_INFINITE, alpha, beta));
                return -ANKA_INFINITE;
            }
            else {
                //io::IndentedPrint(pos.SearchDepth(), "AB result terminal %d\n", Clamp(0, alpha, beta));
                return 0;
            }
        }


        int killer_move_1 = KillerMoves[depth][0];
        int killer_move_2 = KillerMoves[depth][1];
        for (int j = 0; j < list.length; j++) {
            if (list.moves[j].move == hash_move) {
                // TODO: bring hash move to list index 0
                list.moves[j].score = move::PV_SCORE;
            }
            else if ((list.moves[j].move == killer_move_1) || (list.moves[j].move == killer_move_2)) {
                list.moves[j].score = move::KILLER_SCORE;
            }
        }

        int best_score = -ANKA_INFINITE;
        Move best_move = 0;
        while (list.length > 0) {
            Move move = list.PopBest();
            nodes_visited++;
            pos.MakeMove(move);
            int score = -AlphaBeta(pos, -beta, -alpha, depth - 1, params);
            pos.UndoMove();

            if (score > best_score) {
                if (score >= beta) {
                    trans_table.Put(pos_key, NodeType::LOWERBOUND, depth, move, score);
                    if (!in_check && move::IsQuiet(move))
                        SetKillerMove(move, depth);
                    return score;
                }
                else if (score > alpha) {
                    alpha = score;
                }
                best_score = score;
                best_move = move;
            }


            if (params.uci_stop_flag)
                return best_score;
        }
        

        if (best_score > old_alpha) {
            ANKA_ASSERT(best_score < beta);
            trans_table.Put(pos_key, NodeType::PV, depth, best_move, best_score);
        }
        else {
            trans_table.Put(pos_key, NodeType::UPPERBOUND, depth, best_move, best_score);
        }


        return best_score;

    }


    void SearchInstance::CheckStopConditions(SearchParams& params)
    {
        char buffer[4096];
        if (io::PolledRead(buffer, 4096) > 0) {
            if (strncmp(buffer, "stop", 4) == 0) {
                params.uci_stop_flag = true;
                return;
            }
            else if (strncmp(buffer, "quit", 4) == 0) {
                params.uci_stop_flag = true;
                params.uci_quit_flag = true;
                return;
            }
        }

        if (params.check_timeup) {
            auto curr_time = Timer::GetTimeInMs();
            auto time_passed = curr_time - last_timecheck;
            last_timecheck = curr_time;

            params.movetime -= time_passed;

            if (params.movetime < 0)
                params.uci_stop_flag = true;
        }
    }




}