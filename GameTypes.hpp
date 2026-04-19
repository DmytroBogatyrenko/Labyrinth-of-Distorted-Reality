#pragma once

#include <SFML/Graphics.hpp>

struct KeyInfo {
    sf::Vector2i position;
    bool collected = false;
    bool revealed = false;
};

struct EnemyInfo {
    sf::Vector2f position;
    float speed = 1.6F;
    bool chasing = false;
    float attackCooldown = 0.0F;
    float animPhase = 0.0F;
    int state = 0; // 0-wander, 1-alert, 2-chase, 3-search, 4-attack
    float stateTimer = 0.0F;
    sf::Vector2f wanderTarget{0.0F, 0.0F};
    sf::Vector2f lastSeenPlayerPos{0.0F, 0.0F};
    bool hasLastSeen = false;
    bool visibleToPlayer = false;
};