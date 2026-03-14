#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

#include "GameTypes.hpp"

class LabyrinthGame {
public:
    LabyrinthGame();
    void run();

private:
    // ===== [Базові налаштування гри] =====
    static constexpr int chunkSize = 16;
    static constexpr int chunkGrid = 2;
    static constexpr int mapWidth = chunkSize * chunkGrid;
    static constexpr int mapHeight = chunkSize * chunkGrid;
    static constexpr unsigned int screenWidth = 1100;
    static constexpr unsigned int screenHeight = 700;

    // ===== [Налаштування рейтрейсу (огляду від 1-ї особи)] =====
    static constexpr float fov = 3.14159F / 3.0F;
    static constexpr float maxDepth = 24.0F;

    sf::RenderWindow window;
    sf::Clock deltaClock;
    sf::Clock portalClock;

    std::vector<std::string> map;
    std::vector<KeyInfo> keys;

    // ===== [Стан гравця] =====
    sf::Vector2f player{1.5F, 1.5F};
    float playerAngle = 0.0F;
    float walkWavePhase = 0.0F;
    float idleSwayPhase = 0.0F;
    float cameraBobOffset = 0.0F;

    // ===== [Стан проходження] =====
    int score = 0;
    bool exitSpawned = false;
    bool gameWon = false;
    bool showFullMap = false;

    sf::Font font;
    bool fontLoaded = false;

    // ===== [Утиліти карти] =====
    bool isInsideMap(int x, int y) const;
    char tileAt(int x, int y) const;
    bool isBlockingTile(char tile) const;
    int countWallNeighbors(int x, int y) const;

    // ===== [Генерація контенту] =====
    void buildLargeMap();
    void placeKeysRandomly();

    // ===== [Геймплей] =====
    void processEvents();
    void update(float dt);
    void movePlayer(const sf::Vector2f& dir, float distanceStep);
    void revealNearbyKeys();
    void collectAtPlayerCell();
    void unlockDoorAndSpawnExit();
    void checkWin();

    // ===== [Рендер] =====
    sf::Color makeSimpleWallColor(float distanceToWall, char hitTile) const;
    void drawFirstPersonWorld();
    void drawMiniMap();
    void drawFullMapOverlay();
    void drawHud();
    void drawPortalScreen();
    void render();
};