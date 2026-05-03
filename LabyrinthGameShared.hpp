#pragma once

#include <SFML/Graphics.hpp>
#include <string>

namespace LabyrinthGameShared {
enum EnemyState : int {
    EnemyWander = 0,
    EnemyAlert  = 1,
    EnemyChase  = 2,
    EnemySearch = 3,
    EnemyAttack = 4
};

inline sf::String utf8(const std::string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}
}