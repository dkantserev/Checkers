#pragma once

enum class Response
{
    OK, // игра окончена
    BACK, // вернуться на ход назад
    REPLAY, //перезапуск игры
    QUIT, // выход из игры
    CELL // клик по клетке
};
