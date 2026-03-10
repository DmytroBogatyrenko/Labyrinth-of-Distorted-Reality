#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct KeyInfo {
    sf::Vector2i position;
    bool collected = false;
    bool revealed = false;
};

class LabyrinthGame {
public:
    LabyrinthGame()
        : window(sf::VideoMode({screenWidth, screenHeight}), "Лабіринт Спотвореної Реальності - First Person (SFML)") {
        window.setFramerateLimit(60);

        // ===== [Налаштування карти гри] =====
        map = {
            "################",
            "#......#.......#",
            "#.####.#.#####.#",
            "#.#..#.#.....#.#",
            "#.#1.#.###.#.#.#",
            "#.#..#...#.#...#",
            "#.####.#.#.###.#",
            "#......#.#...#.#",
            "#.######.###.#.#",
            "#....2....#.#..#",
            "#.######.#.#.###",
            "#.#....#.#.#...#",
            "#.#.##.#.#.###.#",
            "#...##...#..3D.#",
            "#...........e..#",
            "################",
        };

        keys = {
            KeyInfo{{3, 4}},
            KeyInfo{{5, 9}},
            KeyInfo{{12, 13}},
        };

        // ===== [Налаштування шрифту HUD] =====
        fontLoaded = font.openFromFile("arial.ttf");
        if (!fontLoaded) {
            std::cerr << "[Попередження] Не знайдено arial.ttf. Текст HUD буде вимкнено.\n";
        }
    }

    void run() {
        while (window.isOpen()) {
            const float dt = deltaClock.restart().asSeconds();
            processEvents();
            update(dt);
            render();
        }
    }

private:
    // ===== [Базові налаштування гри] =====
    static constexpr int mapSize = 16;
    static constexpr unsigned int screenWidth = 1100;
    static constexpr unsigned int screenHeight = 700;

    // ===== [Налаштування рейтрейсу (огляду від 1-ї особи)] =====
    static constexpr float fov = 3.14159F / 3.0F;
    static constexpr float maxDepth = 18.0F;

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

    sf::Font font;
    bool fontLoaded = false;

    bool isInsideMap(int x, int y) const {
        return x >= 0 && x < mapSize && y >= 0 && y < mapSize;
    }

    char tileAt(int x, int y) const {
        return map[y][x];
    }

    bool isBlockingTile(char tile) const {
        if (tile == '#') {
            return true;
        }
        if (tile == 'D' && score < 3) {
            return true;
        }
        return false;
    }

    // ===== [Обробка вводу/подій] =====
    void processEvents() {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return;
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }
    }

    // ===== [Оновлення гри за кадр] =====
    void update(float dt) {
        if (gameWon) {
            return;
        }

        // ===== [Налаштування руху гравця] =====
        const float rotationSpeed = 1.8F;
        const float moveSpeed = 3.0F;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            playerAngle -= rotationSpeed * dt;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            playerAngle += rotationSpeed * dt;
        }

        sf::Vector2f moveDir{0.F, 0.F};
        const sf::Vector2f forward{std::cos(playerAngle), std::sin(playerAngle)};

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            moveDir += forward;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            moveDir -= forward;
        }

        const bool isWalking = (moveDir.x != 0.F || moveDir.y != 0.F);
        movePlayer(moveDir, moveSpeed * dt);

        // ===== [Пружиниста хода / хвиля камери] =====
        if (isWalking) {
            walkWavePhase += dt * 10.5F;
            cameraBobOffset = std::sin(walkWavePhase) * 6.0F;
        } else {
            // ===== [Легке коливання в спокої: ефект "паморочиться голова"] =====
            idleSwayPhase += dt * 1.9F;
            cameraBobOffset = std::sin(idleSwayPhase) * 1.4F;
        }

        revealNearbyKeys();
        collectAtPlayerCell();
        unlockDoorAndSpawnExit();
        checkWin();
    }

    void movePlayer(const sf::Vector2f& dir, float distanceStep) {
        if (dir.x == 0.F && dir.y == 0.F) {
            return;
        }

        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        const sf::Vector2f normalized = dir / len;
        const sf::Vector2f candidate = player + normalized * distanceStep;

        const int testX = static_cast<int>(candidate.x);
        const int testY = static_cast<int>(candidate.y);
        if (!isInsideMap(testX, testY)) {
            return;
        }

        const char tile = tileAt(testX, testY);
        if (isBlockingTile(tile)) {
            return;
        }

        player = candidate;
    }

    // ===== [Невидимі ключі: стають видимі коли підійшов близько] =====
    void revealNearbyKeys() {
        for (auto& key : keys) {
            if (key.collected) {
                continue;
            }

            const int px = static_cast<int>(player.x);
            const int py = static_cast<int>(player.y);
            const int dx = std::abs(px - key.position.x);
            const int dy = std::abs(py - key.position.y);
            if (dx <= 1 && dy <= 1) {
                key.revealed = true;
            }
        }
    }

    // ===== [Збір ключа на поточній клітинці] =====
    void collectAtPlayerCell() {
        const int px = static_cast<int>(player.x);
        const int py = static_cast<int>(player.y);
        char& tile = map[py][px];

        if (tile >= '1' && tile <= '3') {
            ++score;
            tile = '.';
            for (auto& key : keys) {
                if (key.position == sf::Vector2i{px, py}) {
                    key.collected = true;
                }
            }
        }
    }

    // ===== [Коли 3 ключі зібрано: двері відкриваються, вихід активується] =====
    void unlockDoorAndSpawnExit() {
        if (score != 3 || exitSpawned) {
            return;
        }

        exitSpawned = true;
        for (auto& row : map) {
            std::replace(row.begin(), row.end(), 'D', '.');
            std::replace(row.begin(), row.end(), 'e', 'E');
        }
    }

    void checkWin() {
        const int px = static_cast<int>(player.x);
        const int py = static_cast<int>(player.y);
        if (map[py][px] == 'E') {
            gameWon = true;
        }
    }

    // ===== [Колір стін: легкий, без важких шейдерів] =====
    sf::Color makeSimpleWallColor(float distanceToWall, char hitTile) const {
        if (hitTile == 'D') {
            const int rust = std::max(35, 145 - static_cast<int>(distanceToWall * 8.F));
            return sf::Color(rust, rust / 2, rust / 3);
        }

        // Простий "нічний" стиль з кроками тону (без шуму/текстур, щоб не лагало).
        const float light = std::clamp(1.0F - (distanceToWall / maxDepth), 0.0F, 1.0F);
        const int tone = static_cast<int>(light * 4.0F); // 0..4

        const int r = 24 + tone * 18;
        const int g = 27 + tone * 18;
        const int b = 33 + tone * 20;
        return sf::Color(r, g, b);
    }

    // ===== [Рендер сцени від 1-ї особи] =====
    void drawFirstPersonWorld() {
        sf::RectangleShape strip;

        const float horizonY = static_cast<float>(screenHeight) / 2.F + cameraBobOffset;

        // Небо
        sf::RectangleShape sky(sf::Vector2f(static_cast<float>(screenWidth), horizonY));
        sky.setFillColor(sf::Color(206, 206, 210));
        window.draw(sky);

        // Підлога (темний брук)
        sf::RectangleShape ground(sf::Vector2f(static_cast<float>(screenWidth), static_cast<float>(screenHeight) - horizonY));
        ground.setPosition(sf::Vector2f(0.F, horizonY));
        ground.setFillColor(sf::Color(53, 53, 58));
        window.draw(ground);

        // Швидший рендер: одна вертикальна смуга на промінь.
        for (unsigned int x = 0; x < screenWidth; ++x) {
            const float rayAngle = (playerAngle - fov / 2.F) + (static_cast<float>(x) / static_cast<float>(screenWidth)) * fov;
            const sf::Vector2f rayDir{std::cos(rayAngle), std::sin(rayAngle)};

            float distanceToWall = 0.F;
            char hitTile = '.';

            while (distanceToWall < maxDepth) {
                distanceToWall += 0.03F;

                const int testX = static_cast<int>(player.x + rayDir.x * distanceToWall);
                const int testY = static_cast<int>(player.y + rayDir.y * distanceToWall);

                if (!isInsideMap(testX, testY)) {
                    distanceToWall = maxDepth;
                    break;
                }

                const char tile = tileAt(testX, testY);
                if (isBlockingTile(tile)) {
                    hitTile = tile;
                    break;
                }
            }

            const float correctedDistance = std::max(0.001F, distanceToWall * std::cos(rayAngle - playerAngle));
            const int wallHeight = static_cast<int>(static_cast<float>(screenHeight) / correctedDistance);
            const int ceiling = std::max(0, static_cast<int>(horizonY) - wallHeight / 2);
            const int floor = std::min(static_cast<int>(screenHeight), ceiling + wallHeight);

            strip.setPosition(sf::Vector2f(static_cast<float>(x), static_cast<float>(ceiling)));
            strip.setSize(sf::Vector2f(1.F, static_cast<float>(std::max(0, floor - ceiling))));
            strip.setFillColor(makeSimpleWallColor(distanceToWall, hitTile));
            window.draw(strip);
        }
    }

    // ===== [Мінікарта (лівий верхній кут)] =====
    void drawMiniMap() {
        constexpr float miniTile = 10.F;
        constexpr float offsetX = 12.F;
        constexpr float offsetY = 12.F;

        sf::RectangleShape tile(sf::Vector2f(miniTile - 1.F, miniTile - 1.F));
        for (int y = 0; y < mapSize; ++y) {
            for (int x = 0; x < mapSize; ++x) {
                char t = map[y][x];
                if (t >= '1' && t <= '3') {
                    bool revealed = false;
                    for (const auto& key : keys) {
                        if (key.position == sf::Vector2i{x, y}) {
                            revealed = key.revealed;
                            break;
                        }
                    }
                    if (!revealed) {
                        t = '.';
                    }
                }

                if (t == '#') {
                    tile.setFillColor(sf::Color(42, 42, 48));
                } else if (t == 'D') {
                    tile.setFillColor(sf::Color(120, 70, 30));
                } else if (t >= '1' && t <= '3') {
                    tile.setFillColor(sf::Color::Yellow);
                } else if (t == 'E') {
                    tile.setFillColor(sf::Color(155, 70, 220));
                } else {
                    tile.setFillColor(sf::Color(180, 180, 180));
                }

                tile.setPosition(sf::Vector2f(offsetX + x * miniTile, offsetY + y * miniTile));
                window.draw(tile);
            }
        }

        sf::CircleShape p(3.F);
        p.setFillColor(sf::Color::Cyan);
        p.setOrigin(p.getGeometricCenter());
        p.setPosition(sf::Vector2f(offsetX + player.x * miniTile, offsetY + player.y * miniTile));
        window.draw(p);

        sf::Vertex line[2];
        line[0].position = sf::Vector2f(offsetX + player.x * miniTile, offsetY + player.y * miniTile);
        line[0].color = sf::Color::Cyan;
        line[1].position = sf::Vector2f(
            offsetX + (player.x + std::cos(playerAngle) * 1.3F) * miniTile,
            offsetY + (player.y + std::sin(playerAngle) * 1.3F) * miniTile
        );
        line[1].color = sf::Color::Cyan;
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    // ===== [HUD: правий верхній кут = ключі] =====
    void drawHud() {
        constexpr float panelW = 220.F;
        constexpr float panelH = 72.F;
        const float panelX = static_cast<float>(screenWidth) - panelW - 16.F;
        const float panelY = 16.F;

        sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
        panel.setPosition(sf::Vector2f(panelX, panelY));
        panel.setFillColor(sf::Color(8, 8, 12, 220));
        panel.setOutlineThickness(1.5F);
        panel.setOutlineColor(sf::Color(90, 90, 105));
        window.draw(panel);

        for (int i = 0; i < 3; ++i) {
            sf::CircleShape orb(10.F);
            orb.setOrigin(orb.getGeometricCenter());
            orb.setPosition(sf::Vector2f(panelX + 30.F + i * 30.F, panelY + panelH / 2.F + 10.F));
            orb.setFillColor(i < score ? sf::Color(235, 205, 70) : sf::Color(55, 55, 60));
            orb.setOutlineThickness(1.F);
            orb.setOutlineColor(sf::Color(130, 130, 140));
            window.draw(orb);
        }

        if (!fontLoaded) {
            return;
        }

        sf::Text corner(font);
        corner.setCharacterSize(18);
        corner.setFillColor(sf::Color(220, 220, 230));
        corner.setPosition(sf::Vector2f(panelX + 16.F, panelY + 10.F));
        corner.setString("КЛЮЧІ: " + std::to_string(score) + "/3");
        window.draw(corner);

        sf::Text controls(font);
        controls.setCharacterSize(22);
        controls.setFillColor(sf::Color::White);
        controls.setPosition(sf::Vector2f(10.F, static_cast<float>(screenHeight - 44)));
        controls.setString("W/S - рух, A/D - поворот, ESC - вихід");
        window.draw(controls);
    }

    // ===== [Екран перемоги / портал] =====
    void drawPortalScreen() {
        sf::RectangleShape bg(sf::Vector2f(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));
        bg.setFillColor(sf::Color(8, 8, 20));
        window.draw(bg);

        const float t = portalClock.getElapsedTime().asSeconds();
        const sf::Vector2f center{screenWidth / 2.F, screenHeight / 2.F - 50.F};
        for (int i = 0; i < 8; ++i) {
            const float radius = 30.F + i * 26.F + std::sin(t * 2.F + i) * 6.F;
            sf::CircleShape ring(radius);
            ring.setOrigin(ring.getGeometricCenter());
            ring.setPosition(center);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.F);
            ring.setOutlineColor(sf::Color(95 + i * 18, 40 + i * 17, 220 - i * 15));
            window.draw(ring);
        }

        if (!fontLoaded) {
            return;
        }

        sf::Text title(font, "СИСТЕМА ЗЛАМАННЯ. ВИ ВІЛЬНІ", 42);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        title.setPosition(sf::Vector2f(120.F, screenHeight - 170.F));
        window.draw(title);

        sf::Text tip(font, "Натисни ESC, щоб закрити гру", 26);
        tip.setFillColor(sf::Color(220, 220, 230));
        tip.setPosition(sf::Vector2f(300.F, screenHeight - 115.F));
        window.draw(tip);
    }

    // ===== [Головний рендер кадру] =====
    void render() {
        window.clear();

        if (gameWon) {
            drawPortalScreen();
        } else {
            drawFirstPersonWorld();
            drawMiniMap();
            drawHud();
        }

        window.display();
    }
};

int main() {
    LabyrinthGame game;
    game.run();
    return 0;
}