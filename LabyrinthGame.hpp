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
    // Тимчасовий тест: після входу з меню одразу показує екран перемоги.
    // Перед показом гри поставити false, щоб повернути нормальний геймплей.
    // static constexpr bool debugStartWithVictoryScreen = true;

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
    sf::Texture flashlightItemTexture;
    sf::Texture knifeItemTexture;
    bool        flashlightItemLoaded = false;
    bool        knifeItemLoaded = false;

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
    float victoryScreenTimer = 0.0F;
    static constexpr float victoryScreenDuration = 10.0F;

    // ===== [Атмосфера / темрява] =====
    float darknessFlicker      = 0.0F;
    float flickerPhase         = 0.0F;
    float ambientDarknessAlpha = 0.0F;
    float pickupTransitionTimer = 0.0F;
    bool interactionPressed = false;
    bool attackPressed = false;
    bool hasFlashlight = false;
    bool hasKnife = false;
    float flashlightBeamStrength = 0.0F;
    float flashlightStableTimer = 0.0F;
    float flashlightBlinkTimer = 0.0F;

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
    sf::SoundBuffer          attackSoundBuffer;
    std::optional<sf::Sound> attackSound;
    sf::SoundBuffer          enemyDeathSoundBuffer;
    std::optional<sf::Sound> enemyDeathSound;
    std::optional<sf::Music> gameMusic;

    // ===== [Приватні методи логіки] =====
    // Обробляє події клавіатури/миші та натискання на кнопки кінцевих екранів
    void processEvents();
    // Оновлює ігрову логіку за кадр
    void update(float dt);
    // Відмальовує поточний кадр
    void render();

    // Завантажує всі звуки гри
    void loadSounds();
    // Завантажує текстури предметів на мапі
    void loadItemTextures();
    // Керує звуком кроків залежно від руху
    void updateFootsteps(float dt, bool isWalking, bool isSprinting);
    // Скидає таймер випадкового скрімера
    void resetScreamerTimer();
    // Оновлює стан і показ скрімера
    void updateScreamer(float dt);
    // Оновлює fade-перехід між станами
    void updateTransition(float dt);

    // ===== [Утиліти карти] =====
    // Перевіряє, чи координати лежать в межах мапи
    bool isInsideMap(int x, int y) const;
    // Повертає символ клітинки мапи.
    char tileAt(int x, int y) const;
    // Перевіряє, чи блокує клітинка рух/промінь
    bool isBlockingTile(char tile) const;
    // Рахує кількість сусідніх стін для генерації
    int  countWallNeighbors(int x, int y) const;

    // ===== [Генерація контенту] =====
    // Генерує велику мапу лабіринту
    void buildLargeMap();
    // Розставляє ключі випадково
    void placeKeysRandomly();
    // Спавнить ворогів у випадкових точках
    void spawnEnemiesRandomly();
    // Завантажує набір кадрів анімації ворога
    bool loadEnemyFrameSet(const std::string& patternPrefix, int count, std::vector<sf::Texture>& outFrames);
    // Завантажує всі графічні ассети ворогів
    void loadEnemySpriteAssets();

    // ===== [Геймплейні механіки] =====
    // Рухає гравця з перевіркою колізій
    void movePlayer(const sf::Vector2f& dir, float distanceStep);
    // Відкриває ключі поруч з гравцем.
    void revealNearbyKeys();
    // Підбирає предмет/ключ у клітинці гравця
    void collectAtPlayerCell();
    // Розміщує стартові предмети (ліхтар, ніж)
    void placeStartingItems();
    // Обробляє взаємодію гравця (клавіша E)
    void handleInteraction();
    // Обробляє атаку гравця
    void handleCombat();
    // Намагається завантажити текстуру рук із кандидатів
    bool tryLoadHandsTexture(const std::vector<std::string>& candidates);
    // Оновлює текстуру рук залежно від інвентаря
    void refreshHandsTexture();
    // Відчиняє двері та спавнить вихід після збору ключів
    void unlockDoorAndSpawnExit();
    // Перевіряє умову перемоги
    void checkWin();
    // Ініціалізує кнопки на екранах завершення
    void initEndButtons();
    // Центрує та задає напис кнопки завершення
    void centerEndButtonLabel(EndButton& button, const std::string& text, float centerX, float centerY, unsigned int size);
        // Запускає новий забіг так само, як при старті через меню.
    void startFreshRun();
    // Повністю скидає стан гри для нового проходження.
    void resetGameState();
    // Оновлює AI та рух ворогів
    void updateEnemies(float dt);
    // Перевіряє видимість між двома точками
    bool hasLineOfSight(const sf::Vector2f& from, const sf::Vector2f& to, float step) const;
    // Перевіряє, чи клітинка доступна для ворога
    bool isWalkableEnemyCell(int x, int y) const;
    // Рухає ворога в напрямку цілі
    void moveEnemyToward(EnemyInfo& enemy, const sf::Vector2f& target, float dt, float speedScale = 1.0F);
    // Вибирає випадкову ціль для блукання ворога
    sf::Vector2f chooseEnemyWanderTarget(const sf::Vector2f& origin) const;

    // ===== [Рендер методи] =====
    // Розраховує колір стіни за дистанцією/типом тайла
    sf::Color makeSimpleWallColor(float distanceToWall, char hitTile) const;
    // Малює 3D-світ від першої особи
    void drawFirstPersonWorld();
    // Малює руки гравця з анімацією
    void drawPlayerHands(bool isWalking, bool isSprinting);
    // Малює віньєтку по краях екрана
    void drawVignette();
    // Малює мінімапу
    void drawMiniMap();
    // Малює повноекранну карту
    void drawFullMapOverlay();
    // Малює HUD (HP, витривалість, підказки)
    void drawHud();
    // Малює порталний екран
    void drawPortalScreen();
    // Малює екран поразки
    void drawGameOver();
    // Малює екран перемоги
    void drawVictoryScreen();
    // Малює затемнення переходу
    void drawTransitionOverlay();
    // Малює скрімер поверх сцени
    void drawScreamer();
};