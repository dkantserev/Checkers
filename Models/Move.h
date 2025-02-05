#pragma once
#include <stdlib.h>

typedef int8_t POS_T;

// структура, описывающая ход в игре
struct move_pos
{
    POS_T x, y;             // начальная позиция (откуда выполняется ход)
    POS_T x2, y2;           // конечная позиция (куда выполняется ход)
    POS_T xb = -1, yb = -1; // координаты побитой фигуры

    // конструктор для обычного хода
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    // конструктор для хода с взятием фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other);
    }
};
