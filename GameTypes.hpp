#pragma once

#include <SFML/Graphics.hpp>

struct KeyInfo {
    sf::Vector2i position;
    bool collected = false;
    bool revealed = false;
};