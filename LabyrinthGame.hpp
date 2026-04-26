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

    // ===== [Ядро SFML] =====
    sf::RenderWindow window;
    sf::Clock deltaClock;
    sf::Clock portalClock;
    sf::Font font;
    bool fontLoaded = false;

    // ===== [Дані світу] =====
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

    // ===== [Текстури та Спрайти (Важливий порядок!)] =====
    // Спершу текстури, потім спрайти
    sf::Texture handsTexture;
    sf::Sprite  handsSprite;
    bool        handsLoaded = false;
    float       handSwayPhase = 0.0F;
    float       handBobY      = 0.0F;
    float       handBobX      = 0.0F;
    float       handIdlePhase = 0.0F;

    sf::Texture screamerTexture;
    sf::Sprite  screamerSprite;
    bool        screamerLoaded = false;

    // ===== [Стан проходження] =====
    int  score       = 0;
    bool exitSpawned = false;
    bool gameWon     = false;
    bool gameover    = false;
    bool showFullMap = false;

        enum class EndScreen { None, GameOver, Victory };
    enum class TransitionState { Idle, FadeOut, FadeIn };

    EndScreen       activeEndScreen  = EndScreen::None;
    EndScreen       pendingEndScreen = EndScreen::None;
    TransitionState transitionState  = TransitionState::FadeIn;
    float           transitionAlpha  = 255.0F;
    float           transitionSpeed  = 145.0F;

    struct EndButton {
        sf::RectangleShape box;
        std::optional<sf::Text> label;
    };

    EndButton restartButton;
    EndButton menuButton;

    // ===== [Атмосфера / темрява] =====
    float darknessFlicker      = 0.0F;
    float flickerPhase         = 0.0F;
    float ambientDarknessAlpha = 0.0F;

    // ===== [Вороги] =====
    bool enemySpriteAssetsLoaded = false;
    std::vector<sf::Texture> enemyWalkFrames;
    std::vector<sf::Texture> enemyAttackFrames;
    std::vector<sf::Texture> enemyAlertFrames;

    // ===== [Логіка Скрімера] =====
    bool  screamerActive               = false;
    float screamerShowTimer            = 0.0F;
    float screamerTimeSinceLastTrigger = 0.0F;
    float screamerNextTrigger          = 30.0F;
    static constexpr float screamerShowDuration = 0.70F;

    // ===== [Аудіо] =====
    sf::SoundBuffer          screamerSoundBuffer;
    std::optional<sf::Sound> screamerSound;

    sf::SoundBuffer          footstepSoundBuffer;
    std::optional<sf::Sound> footstepSound;
    float footstepTimer = 0.0F;
    static constexpr float footstepInterval = 0.38F;

    sf::SoundBuffer          pickupSoundBuffer;
    std::optional<sf::Sound> pickupSound;

    // ===== [Приватні методи логіки] =====
    void processEvents();
    void update(float dt);
    void render();

    void loadSounds();
    void updateFootsteps(float dt, bool isWalking, bool isSprinting);
    void resetScreamerTimer();
    void updateScreamer(float dt);
    void updateTransition(float dt);

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

    // ===== [Геймплейні механіки] =====
    void movePlayer(const sf::Vector2f& dir, float distanceStep);
    void revealNearbyKeys();
    void collectAtPlayerCell();
    void unlockDoorAndSpawnExit();
    void checkWin();
    void initEndButtons();
    void centerEndButtonLabel(EndButton& button, const std::string& text, float centerX, float centerY, unsigned int size);
    void resetGameState();
    void updateEnemies(float dt);
    bool hasLineOfSight(const sf::Vector2f& from, const sf::Vector2f& to, float step) const;
    bool isWalkableEnemyCell(int x, int y) const;
    void moveEnemyToward(EnemyInfo& enemy, const sf::Vector2f& target, float dt, float speedScale = 1.0F);
    sf::Vector2f chooseEnemyWanderTarget(const sf::Vector2f& origin) const;

    // ===== [Рендер методи] =====
    sf::Color makeSimpleWallColor(float distanceToWall, char hitTile) const;
    void drawFirstPersonWorld();
    void drawPlayerHands(bool isWalking, bool isSprinting);
    void drawVignette();
    void drawMiniMap();
    void drawFullMapOverlay();
    void drawHud();
    void drawPortalScreen();
    void drawGameOver();
    void drawVictoryScreen();
    void drawTransitionOverlay();
    void drawScreamer();
};