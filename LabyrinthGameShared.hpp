#pragma once

#include <SFML/Graphics.hpp>
#include <string>

namespace LabyrinthGameShared {
// поточний стан ворога (блукає, помітив, переслідує, шукає, атакує)
enum EnemyState : int {
    EnemyWander = 0,
    EnemyAlert  = 1,
    EnemyChase  = 2,
    EnemySearch = 3,
    EnemyAttack = 4
};

// функція перетворює std::string (UTF-8) у sf::String для коректного показу українського тексту
inline sf::String utf8(const std::string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}
}