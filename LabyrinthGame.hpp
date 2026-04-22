#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
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
    static constexpr int mapWidth  = chunkSize * chunkGrid;
    static constexpr int mapHeight = chunkSize * chunkGrid;
    static constexpr unsigned int screenWidth  = 1100;
    static constexpr unsigned int screenHeight = 700;

    // ===== [Налаштування рейтрейсу] =====
    static constexpr float fov      = 3.14159F / 3.0F;
    static constexpr float maxDepth = 24.0F;

    sf::RenderWindow window;
    sf::Clock deltaClock;
    sf::Clock portalClock;

    std::vector<std::string> map;
    std::vector<KeyInfo>     keys;
    std::vector<EnemyInfo>   enemies;

    // ===== [Стан гравця] =====
    sf::Vector2f player{1.5F, 1.5F};
    float playerAngle       = 0.0F;
    float walkWavePhase     = 0.0F;
    float idleSwayPhase     = 0.0F;
    float cameraBobOffset   = 0.0F;
    float hp                = 100.0F;
    float stamina           = 100.0F;
    float flightVisualTimer = 0.0F;
    float sprintVisualTimer = 0.0F;

    // ===== [Руки гравця] =====
    float handSwayPhase = 0.0F;
    float handBobY      = 0.0F;
    float handBobX      = 0.0F;
    float handIdlePhase = 0.0F;

    // ===== [Стан проходження] =====
    int  score       = 0;
    bool exitSpawned = false;
    bool gameWon     = false;
    bool gameover    = false;
    bool showFullMap = false;

    // ===== [Атмосфера / темрява] =====
    float darknessFlicker      = 0.0F;
    float flickerPhase         = 0.0F;
    float ambientDarknessAlpha = 0.0F;

    sf::Font font;
    bool fontLoaded              = false;
    bool enemySpriteAssetsLoaded = false;
    std::vector<sf::Texture> enemyWalkFrames;
    std::vector<sf::Texture> enemyAttackFrames;
    std::vector<sf::Texture> enemyAlertFrames;

    // ===== [Скрімер] =====
    sf::Texture screamerTexture;
    bool  screamerLoaded               = false;
    bool  screamerActive               = false;
    float screamerShowTimer            = 0.0F;
    float screamerTimeSinceLastTrigger = 0.0F;
    float screamerNextTrigger          = 30.0F;
    static constexpr float screamerShowDuration = 0.70F;

    void resetScreamerTimer();
    void updateScreamer(float dt);
    void drawScreamer();

    // ===== [Аудіо] =====
    sf::SoundBuffer          screamerSoundBuffer;
    std::optional<sf::Sound> screamerSound;

    sf::SoundBuffer          footstepSoundBuffer;
    std::optional<sf::Sound> footstepSound;
    float footstepTimer = 0.0F;
    static constexpr float footstepInterval = 0.38F;

    sf::SoundBuffer          pickupSoundBuffer;
    std::optional<sf::Sound> pickupSound;

    void loadSounds();
    void updateFootsteps(float dt, bool isWalking, bool isSprinting);

    // ===== [Утиліти карти] =====
    bool isInsideMap(int x, int y) const;
    char tileAt(int x, int y) const;
    bool isBlockingTile(char tile) const;
    int  countWallNeighbors(int x, int y) const;

    // ===== [Генерація контенту] =====
    void buildLargeMap();
    void placeKeysRandomly();
    void spawnEnemiesRandomly();
    bool loadEnemyFrameSet(const std::string& patternPrefix, int count, std::vector<sf::Texture>& outFrames);
    void loadEnemySpriteAssets();

    // ===== [Геймплей] =====
    void processEvents();
    void update(float dt);
    void movePlayer(const sf::Vector2f& dir, float distanceStep);
    void revealNearbyKeys();
    void collectAtPlayerCell();
    void unlockDoorAndSpawnExit();
    void checkWin();
    void updateEnemies(float dt);
    bool hasLineOfSight(const sf::Vector2f& from, const sf::Vector2f& to, float step) const;
    bool isWalkableEnemyCell(int x, int y) const;
    void moveEnemyToward(EnemyInfo& enemy, const sf::Vector2f& target, float dt, float speedScale = 1.0F);
    sf::Vector2f chooseEnemyWanderTarget(const sf::Vector2f& origin) const;

    // ===== [Рендер] =====
    sf::Color makeSimpleWallColor(float distanceToWall, char hitTile) const;
    void drawFirstPersonWorld();
    void drawPlayerHands(bool isWalking, bool isSprinting);
    void drawVignette();
    void drawMiniMap();
    void drawFullMapOverlay();
    void drawHud();
    void drawPortalScreen();
    void drawGameOver();
    void render();
};