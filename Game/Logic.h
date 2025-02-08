#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }
    
    //возвращает вектор лучших ход для указанного игрока
    vector<move_pos> find_best_turns(const bool color)
    {
        
        next_move.clear();
        next_best_state.clear();
        find_first_best_turn(board->get_board(), color, -1, -1, 0);
        int state = 0;
        vector<move_pos> res;
        do {
            res.push_back(next_move[state]);
            state = next_best_state[state];
        } while (state != -1 && next_move[state].x != -1);

        return res;       
    }

private:
    //выполняет ход на копии доски и возвращает доску после хода
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }
    // оценивает состояние доски для бота
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // всего белых пешек
                wq += (mtx[i][j] == 3); //      белых королев
                b += (mtx[i][j] == 2); //       черных пешек
                bq += (mtx[i][j] == 4); //      черных королев
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4; // вес королевы
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        // инициализация информации о следующем ходе
        next_move.emplace_back(-1, -1, -1, -1);
        next_best_state.push_back(-1);

        //  определяем возможные ходы
        if (state != 0) {
            find_turns(x, y, mtx);
        }
        auto now_turns = turns;
        auto now_have_beats = have_beats;

        // если бить нечего, передаём ход противнику и ищем лучший ответ
        if (!now_have_beats && state != 0) {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        double best_score = -1;

        // перебираем все возможные ходы
        for (auto turn : now_turns) {
            size_t new_state = next_move.size();
            double score;

            // если есть взятие, продолжаем поиск хода для той же фигуры
            if (now_have_beats) {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            }
            // иначе передаём ход противнику
            else {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }

            // обновляем лучший ход
            if (score > best_score) {
                best_score = score;
                next_move[state] = turn;
                next_best_state[state] = (now_have_beats ? new_state : -1);
            }
        }
        return best_score;
    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        // достижение максимальной глубины поиска - оцениваем позицию
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color));
        }

        // если продолжаем ход после взятия, ищем ходы для конкретной фигуры
        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        else {
            find_turns(color, mtx);
        }
        auto now_turns = turns;
        bool now_have_beats = have_beats;

        // если нет возможных ходов, передаём ход противнику
        if (!now_have_beats && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // если у игрока нет ходов, это означает проигрыш
        if (turns.empty())
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1;
        double max_score = -1;

        // перебираем все возможные ходы
        for (auto turn : now_turns)
        {
            double score = 0.0;

            // если нет взятия и ход только начинается, передаём ход противнику
            if (!now_have_beats && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            // если ход продолжается, ищем следующий ход для той же фигуры
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            // обновляем минимальную и максимальную оценки хода
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // Альфа-бета отсечение
            if (depth % 2) {
                alpha = max(alpha, max_score);
            }
            else {
                beta = min(beta, min_score);
            }

            // если находим отсечение, прекращаем перебор
            if (optimization != "O0" && alpha > beta) {
                break;
            }

            // дополнительное отсечение при равенстве альфа и бета
            if (optimization != "O2" && alpha == beta) {
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }
        // возвращаем оптимальный результат для текущего игрока
        return (depth % 2 ? max_score : min_score);
    }

public:
    // ищет возможные ходы для указанного цвета
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }
    // ищет возможные ходы для указанной клетки
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    //основной метод для поиска возможных ходов на доске
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }
    // ищет возможные ходы для указанной фигуры на переданной доске
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    vector<move_pos> turns; //возможные ходы
    bool have_beats; // флаг обязательного взятия шашки
    int Max_depth; // максимальная глубина поиска ходов

  private:
    default_random_engine rand_eng; // генератор случайных чисел
    string scoring_mode;
    string optimization; // оценка позиции бота
    vector<move_pos> next_move; // лучшие ходы
    vector<int> next_best_state; // состояние после выполненого хода
    Board *board; // указатель на Board
    Config *config; // указатель на Config
};
