#ifndef ENGINE_H
#define ENGINE_h

#include "Board/Board.h"
#include "Util/Move.h"
#include <random>
#include <stack>
#include <algorithm>
using namespace std;

/*
 * Represents a chess game, with functions to move chess pieces and uphold the rules of chess (e.g. check, checkmate, turns)
 * Can generate the "best" move for specific color
*/
// TODO: implement undo/history
// TODO: simple move
class ChessEngine {
#pragma region CHESS_ENGINE_PRIVATE
private:
    Board* board;
    Color turn;
    int level;

    // random stuff
    random_device rd;
    mt19937 rng;

    // Returns random integer from [n1, n2)
    int random_number(int n1, int n2) {
        uniform_int_distribution<int> uni(n1, n2 - 1);
        return uni(rng);
    }

    // structure for storing next possible move. Used for move generation
    struct PossibleMove {
        Color color;
        Move root, move, predicted_move;
        int depth, score, best_score, children_count;
        bool visited = false;
        int alpha = INT16_MIN, beta = INT16_MAX;
        PossibleMove* parent;
    };
    // Creates a possible move
    PossibleMove* create_possible_move(Color color, Move root, Move move, int depth, int score, int best_score, int alpha, int beta, PossibleMove* parent) {
        PossibleMove* pm = new PossibleMove();
        pm->color = color; pm->root = root; pm->move = move; pm->depth = depth;
        pm->alpha = alpha; pm->beta = beta;
        pm->score = score; pm->best_score = best_score; pm->parent = parent;
        return pm;
    }
    // comparator used for sorting
    static bool compare_possible_move(PossibleMove* pm, PossibleMove* pm2) {
        return pm->score < pm2->best_score;
    }

    // https://www.chessprogramming.org/Center_Manhattan-Distance
    const int distance_from_center[64] = {
        6, 5, 4, 3, 3, 4, 5, 6,
        5, 4, 3, 2, 2, 3, 4, 5,
        4, 3, 2, 1, 1, 2, 3, 4,
        3, 2, 1, 0, 0, 1, 2, 3,
        3, 2, 1, 0, 0, 1, 2, 3,
        4, 3, 2, 1, 1, 2, 3, 4,
        5, 4, 3, 2, 2, 3, 4, 5,
        6, 5, 4, 3, 3, 4, 5, 6
    };

    /*
     * Checks if a move will result in check
    */
    bool will_check(Vector from, Vector to) { return will_check(from, to, turn); }
    bool will_check(Vector from, Vector to, Color color) {
        if (board->within_boundaries(from) && board->within_boundaries(to)) {
            Piece* p = board->get_piece(from);
            // temporary move piece to new place and check if it results in check
            Piece* old = board->replace_piece(to, p);
            board->clear_piece(from);
            bool in_check = is_check(color);
            // move piece back to original position
            board->replace_piece(to, old);
            board->replace_piece(from, p);
            return in_check;
        }
        return false;
    }

    /*
     * Returns all possible (valid) moves for a specific color
    */
    vector<Move> get_all_valid_moves() { return get_all_valid_moves(turn); }
    vector<Move> get_all_valid_moves(Color color) {
        vector<Move> possible_moves;
        vector<Piece*> pieces = board->get_pieces(color);
        for (auto pa = pieces.begin(); pa != pieces.end(); pa++) {
            Piece* p = *pa;
            vector<Vector> valid_moves = p->get_valid_moves(board);
            Vector from = p->position;
            for (auto m = valid_moves.begin(); m != valid_moves.end(); m++) {
                if (!will_check(from, *m, color)) {
                    possible_moves.push_back(Move(from, *m, p, board->get_piece(*m)));
                }
            }
        }
        return possible_moves;
    }
#pragma endregion CHESS_ENGINE_PRIVATE
#pragma region CHESS_ENGINE_PUBLIC
public:
    ChessEngine() : board(new Board()), turn(WHITE), level(0), rng(mt19937(rd())) {}
    ChessEngine(int level) : board(new Board()), turn(WHITE), level(level), rng(mt19937(rd())) {}

    /*
     * Move piece from (px, py) to (nx, y)
     * Returns true if it's a valid move, or false if the move is invalid or the move would result in a check for the current turn
    */
    bool move_piece(Move move) { return move_piece(move.move_from, move.move_to); }
    bool move_piece(int fx, int fy, int tx, int ty) { return move_piece(Vector(fx, fy), Vector(tx, ty)); }
    bool move_piece(Vector from, Vector to) {
        Piece *p = board->get_piece(from);
        if (p != NULL && p->color == turn && p->is_valid_move(to, board) && !will_check(from, to)) {
            board->replace_piece(to, board->get_piece(from));
            board->clear_piece(from);
            next_turn();
            return true;
        }
        return false;
    }

    /*
     * Checks if the given color (or current turn color if none is given) is in check
    */
    bool is_check() { return is_check(turn); }
    bool is_check(Color color) {
        Vector king_position = board->get_king(color)->position;
        vector<Piece*> pieces = board->get_pieces(get_other_color(color));
        for (auto pa = pieces.begin(); pa != pieces.end(); pa++) {
            vector<Vector> valid_moves = (*pa)->get_valid_moves(board);
            for (auto m = valid_moves.begin(); m != valid_moves.end(); m++) {
                if (m->equal_to(king_position)) {
                    return true;
                }
            }
        }
        return false;
    }

    /*
     * Checks if the given color (or current turn color if none is given) is in checkmate
    */
    bool is_checkmate() { return is_checkmate(turn); }
    bool is_checkmate(Color color) {
        if (is_check(color)) {
            // check if we have a piece that can prevent check
            // this is faster than finding all possible moves first, since here we could return right away
            vector<Piece*> pieces = board->get_pieces(color);
            for (auto pa = pieces.begin(); pa != pieces.end(); pa++) {
                Piece* p = *pa;
                vector<Vector> valid_moves = p->get_valid_moves(board);
                Vector from = p->position;
                for (auto m = valid_moves.begin(); m != valid_moves.end(); m++) {
                    // if we have at least one valid move, that means we can still prevent check
                    if (!will_check(from, *m, color)) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }

    /*
     * Checks if the given color (or current turn color if none is given) is in stalement
    */
    bool is_stalemate() { return is_stalemate(turn); }
    bool is_stalemate(Color color) {
        if (!is_check(color)) {
            vector<Piece *> pieces = board->get_pieces(color);
            for (auto pa = pieces.begin(); pa != pieces.end(); pa++) {
                vector<Vector> valid_moves = (*pa)->get_valid_moves(board);
                // if we have at least 1 valid move, then we're not in stalemate
                if (valid_moves.size() != 0) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /*
     * Randomnly generates the next (valid) move for the given color (or current turn color if none is given)
    */
    Move generate_random_move() { return generate_random_move(turn); }
    Move generate_random_move(Color color) {
        vector<Move> possible_moves = get_all_valid_moves(color);
        return possible_moves.at(random_number(0, possible_moves.size()));
    }

    /*
     * Calculates and returns the next move for the given color (or current turn color if none is given)
     * Utilizes the chess engine level. If level is low, the function takes less time to complete, but may result in worse moves,
     * while higher levels take more time to calculate and may result in better moves
     * 
     * This function implements negamax with alpha-beta pruning
    */
    Move generate_move() { return generate_move(turn); }
    Move generate_move(Color color) {
        if (level == 0) return generate_random_move(color);
        stack<PossibleMove*> pm_stack;
        vector<PossibleMove> best_moves;
        int best_score = INT32_MIN, worst_score = INT32_MAX;
        int root_alpha = -INT16_MAX, root_beta = INT16_MAX;
        Color other_color = get_other_color(color);
        // first add all the first moves
        vector<Move> first_moves = get_all_valid_moves(color);
        int root_children_count = first_moves.size();
        vector<PossibleMove*> first_pms;
        for (auto m = first_moves.begin(); m != first_moves.end(); m++) {
            first_pms.push_back(create_possible_move(
                other_color, *m, *m, 1,
                calculate_utility(*m), INT32_MIN, root_alpha, root_beta, NULL
            ));
        }
        // sort for slightly better pruning
        sort(first_pms.begin(), first_pms.end(), compare_possible_move);
        for (auto pm = first_pms.begin(); pm != first_pms.end(); pm++) {
            pm_stack.push(*pm);
        }
        int count = 0;
        cout << "moves considered: " << count;
        while (!pm_stack.empty()) {
            PossibleMove *pm = pm_stack.top();
            Move move = pm->move;
            cout << '\r' << "moves considered: " << count;
            if (pm->visited) {
                pm_stack.pop();
                // undo move
                board->replace_piece(move.move_to, move.piece_replaced);
                board->replace_piece(move.move_from, move.piece_moved);
                PossibleMove* parent = pm->parent;
                int negated_score = -pm->best_score;
                // update best scores based on negamax
                if (parent != NULL) {
                    parent->children_count--;
                    if (negated_score > parent->best_score) {
                        parent->best_score = negated_score;
                        if (pm->color == color) parent->predicted_move = pm->move;
                    }
                    // alpha beta pruning
                    parent->alpha = max(parent->alpha, parent->best_score);
                    if (parent->alpha > parent->beta) {
                        while (parent->children_count) {
                            delete pm_stack.top();
                            pm_stack.pop();
                            parent->children_count--;
                        }
                    }
                } else {
                    root_children_count--;
                    // update root best scores
                    if (negated_score > best_score) {
                        best_score = negated_score;
                        best_moves.clear();
                        best_moves.push_back(*pm);
                    } else if (negated_score == best_score) {
                        best_moves.push_back(*pm);
                    }
                    // alpha beta pruning, but on the root
                    root_alpha = max(root_alpha, best_score);
                    if (root_alpha > root_beta) {
                        while (root_children_count) {
                            delete pm_stack.top();
                            pm_stack.pop();
                            root_children_count--;
                        }
                    }
                    if (negated_score < worst_score) {
                        worst_score = negated_score;
                    }
                }
                delete pm;
                count++;
            } else {
                // move piece to find new possible moves
                board->replace_piece(move.move_to, move.piece_moved);
                board->clear_piece(move.move_from);
                // mark it visited so we revisit later and undo the move done
                pm->visited = true;
                // if we haven't reached all levels yet, then push all possible moves for next turn
                if (pm->depth != level) {
                    // update to get the latest alphas and betas for pruning
                    if (pm->parent != NULL) {
                        pm->alpha = -pm->parent->beta;
                        pm->beta = -pm->parent->alpha;
                    } else {
                        pm->alpha = -root_beta;
                        pm->beta = -root_alpha;
                    }
                    vector<Move> possible_moves = get_all_valid_moves(pm->color);
                    // used later to keep track of how many of the children are still left in stack
                    pm->children_count = possible_moves.size();
                    // if we have no possible moves, that means we've reached the end of a branch early
                    if (pm->children_count == 0) {
                        // set the best_score with current sign (for negamax) and set its level to the highest (essentially marking it the end of branch)
                        pm->best_score = (pm->color == color ? 1 : -1) * pm->score;
                        pm->depth = level;
                        continue;
                    }
                    Color new_color = get_other_color(pm->color);
                    int sign = pm->color == color ? 1 : -1;
                    vector<PossibleMove*> pms;
                    for (auto m = possible_moves.begin(); m != possible_moves.end(); m++) {
                        int new_score = pm->score + sign * calculate_utility(*m);
                        // temporary move piece to see if it leads to stalemate/checkmate
                        board->replace_piece(m->move_to, m->piece_moved);
                        board->clear_piece(m->move_from);
                        if (is_stalemate(other_color)) {
                            new_score -= INT16_MAX / 64;
                        } else if (is_stalemate(color)) {
                            new_score += INT16_MAX / 64;
                        } else if (is_checkmate(color)) {
                            new_score -= INT16_MAX / 2;
                        } else if (is_checkmate(other_color)) {
                            new_score += INT16_MAX / 2;
                        }
                        // move piece back
                        board->replace_piece(m->move_to, m->piece_replaced);
                        board->replace_piece(m->move_from, m->piece_moved);
                        // push the possible move into stack with its calculated score
                        pms.push_back(create_possible_move(
                            new_color, pm->root, *m, pm->depth + 1, new_score, INT32_MIN, -pm->beta, -pm->alpha, pm
                        ));
                    }
                    // sorting them for slightly better pruning
                    sort(pms.begin(), pms.end(), compare_possible_move);
                    for (auto pm = pms.begin(); pm != pms.end(); pm++) {
                        pm_stack.push(*pm);
                    }
                } else {
                    // we have reached all levels, simply set the best_score with current sign (for negamax)
                    pm->best_score = (pm->color == color ? 1 : -1) * pm->score;
                }
            }
        }
        PossibleMove best_move = best_moves.at(random_number(0, best_moves.size()));
        // PossibleMove best_move = best_moves.at(0);
        cout << endl;
        // cout << "best score: " << best_score << endl;
        // cout << "worse score: " << worst_score << endl;
        // cout << "predicted move: " << best_move.predicted_move.as_string() << endl;
        return best_move.root;
    }

    /*
     * Calculates utility (score) for a given move based several factors
     * - material value of captured (if any) piece
     * - mobility value (how many moves it can take afterwards)
    */
    int calculate_utility(Move m) {
        Piece* moved = m.piece_moved;
        Piece* captured = m.piece_replaced;
        int old_mobility = moved->get_valid_moves(board).size();
        // temporary move piece to check mobility and other factors
        board->replace_piece(m.move_to, moved);
        board->clear_piece(m.move_from);
        int material = captured == NULL ? 0 : captured->get_value();
        int mobility = moved->get_valid_moves(board).size();
        int center_distance = distance_from_center[8 * m.move_from.x + m.move_from.y] - distance_from_center[8 * m.move_to.x + m.move_to.y];
        int piece_square_value = moved->get_square_table_value(is_end_game());
        // move piece back
        board->replace_piece(m.move_to, captured);
        board->replace_piece(m.move_from, moved);
        return 10 * material + 2 * center_distance + 2 * mobility + piece_square_value;
    }

    /*
     * Tries to determine whether the current game is close to the end by checking the difference between the total piece values between
     * the two colors, as well as the total numbeof pieces on each side
    */
    bool is_end_game() {
        vector<Piece*> white_pieces = board->get_pieces(WHITE);
        vector<Piece*> black_pieces = board->get_pieces(BLACK);
        int white_values = 0, black_values = 0;
        for (auto pa = white_pieces.begin(); pa != white_pieces.end(); pa++) {
            Piece* p = *pa;
            if (p->type != KING) white_values += p->get_value();
        }
        for (auto pa = black_pieces.begin(); pa != black_pieces.end(); pa++) {
            Piece* p = *pa;
            if (p->type != KING) black_values += p->get_value();
        }
        int count_diff = white_pieces.size() - black_pieces.size();
        return abs(white_values - black_values) >= 10 || abs(count_diff) > 10;
    }

    // Swap turn to play next move
    void next_turn() { turn = turn == WHITE ? BLACK : WHITE; }
    // Get the current turn's color
    Color get_turn() { return turn; }
    // Get the current enemy's color
    Color get_other_color() { return turn == WHITE ? BLACK : WHITE; }
    // Get the current enemy's color
    Color get_other_color(Color color) { return color == WHITE ? BLACK : WHITE; }

    /*
     * Resets chess game and board state 
    */
    void reset_game() { reset_game(level); }
    void reset_game(int level) {
        // perhaps there's a better way to do this, but im lazy rn lol
        delete board;
        board = new Board();
        turn = WHITE;
        this->level = level;
    }

    // Returns current chess engine level
    int get_level() { return level; }

    /*
     * Prints board state and the current turn
    */
    void print_board() {
        cout << endl;
        board->print_board();
        if (turn == WHITE) cout << "WHITE TURN";
        else cout << "BLACK TURN";
        if (is_checkmate()) cout << ": CHECKMATE. GAME OVER!";
        else if (is_check()) cout << ": CHECK!";
        cout << endl;
    }

    ~ChessEngine() {
        delete board;
    }
#pragma endregion CHESS_ENGINE_PUBLIC
};

#endif
