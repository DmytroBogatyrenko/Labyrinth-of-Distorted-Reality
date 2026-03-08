#include <SFML/Graphics.hpp>

#include <algorithm>
#include <optional>
#include <cmath>
#include <iostream>
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
        : window(sf::VideoMode({windowWidth, windowHeight}), "Лабіринт Спотвореної Реальності - SFML") {
        window.setFramerateLimit(60);

        map = {
            "################",
            "#......#.......#",
            "#.####.#.#####.#",
            "#.#..#.#.....#.#",
            "#..1.#.###.#.#.#",
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

        // Для тексту: спочатку пробуємо локальний файл arial.ttf (поклади поруч з exe).
        // Якщо шрифт не знайдено, гра все одно працює, але без написів.
        fontLoaded = font.openFromFile("arial.ttf");
        if (!fontLoaded) {
            std::cerr << "[Попередження] Не знайдено arial.ttf. Текст у вікні буде вимкнено.\n";
        }
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            update();
            render();
        }
    }

private:
    static constexpr int mapSize = 16;
    static constexpr int tileSize = 42;
    static constexpr unsigned int hudHeight = 120;
    static constexpr unsigned int windowWidth = mapSize * tileSize;
    static constexpr unsigned int windowHeight = mapSize * tileSize + hudHeight;

    sf::RenderWindow window;
    std::vector<std::string> map;
    std::vector<KeyInfo> keys;

    sf::Vector2i player{1, 1};
    int score = 0;
    bool exitSpawned = false;
    bool gameWon = false;

    sf::Font font;
    bool fontLoaded = false;

    bool isInsideMap(int x, int y) const {
        return x >= 0 && x < mapSize && y >= 0 && y < mapSize;
    }

    bool isBlockedTile(char tile) const {
        if (tile == '#') {
            return true;
        }

        if (tile == 'D' && score < 3) {
            return true;
        }

        return false;
    }

    void processEvents() {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return;
            }

            if (gameWon) {
                // Після перемоги закрити вікно можна ESC або хрестиком.
                if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
                continue;
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                handleMoveInput(keyPressed->code);
            }
        }
    }

    void handleMoveInput(sf::Keyboard::Key key) {
        sf::Vector2i next = player;

        if (key == sf::Keyboard::Key::W || key == sf::Keyboard::Key::Up) {
            next.y -= 1;
        } else if (key == sf::Keyboard::Key::S || key == sf::Keyboard::Key::Down) {
            next.y += 1;
        } else if (key == sf::Keyboard::Key::A || key == sf::Keyboard::Key::Left) {
            next.x -= 1;
        } else if (key == sf::Keyboard::Key::D || key == sf::Keyboard::Key::Right) {
            next.x += 1;
        } else if (key == sf::Keyboard::Key::Escape) {
            window.close();
            return;
        } else {
            return;
        }

        if (!isInsideMap(next.x, next.y)) {
            return;
        }

        const char nextTile = map[next.y][next.x];
        if (isBlockedTile(nextTile)) {
            return;
        }

        player = next;

        char& currentTile = map[player.y][player.x];

        // Збір ключа: наступили на 1/2/3 -> ставимо '.' і додаємо score.
        if (currentTile >= '1' && currentTile <= '3') {
            ++score;
            currentTile = '.';
            for (auto& keyInfo : keys) {
                if (keyInfo.position == player) {
                    keyInfo.collected = true;
                }
            }
        }

        // Кінець: дійшли до E.
        if (currentTile == 'E') {
            gameWon = true;
            std::cout << "\nСИСТЕМА ЗЛАМАННЯ. ВИ ВІЛЬНІ\n";
        }
    }

    void update() {
        if (gameWon) {
            return;
        }

        revealNearbyKeys();

        if (score == 3 && !exitSpawned) {
            exitSpawned = true;
            for (auto& row : map) {
                std::replace(row.begin(), row.end(), 'D', '.');
                std::replace(row.begin(), row.end(), 'e', 'E');
            }
        }
    }

    void revealNearbyKeys() {
        for (auto& keyInfo : keys) {
            if (keyInfo.collected) {
                continue;
            }

            const int dx = std::abs(player.x - keyInfo.position.x);
            const int dy = std::abs(player.y - keyInfo.position.y);
            if (dx <= 1 && dy <= 1) {
                keyInfo.revealed = true;
            }
        }
    }

    bool isKeyRevealedAt(int x, int y) const {
        for (const auto& keyInfo : keys) {
            if (keyInfo.position == sf::Vector2i{x, y}) {
                return keyInfo.revealed;
            }
        }
        return true;
    }

    void drawMap() {
        sf::RectangleShape tileShape(sf::Vector2f(static_cast<float>(tileSize - 1), static_cast<float>(tileSize - 1)));

        for (int y = 0; y < mapSize; ++y) {
            for (int x = 0; x < mapSize; ++x) {
                char tile = map[y][x];

                // Невидимі ключі ховаємо до моменту наближення.
                if (tile >= '1' && tile <= '3' && !isKeyRevealedAt(x, y)) {
                    tile = '.';
                }

                if (tile == '#') {
                    tileShape.setFillColor(sf::Color(35, 35, 45));
                } else if (tile == 'D') {
                    tileShape.setFillColor(sf::Color(110, 70, 30));
                } else if (tile >= '1' && tile <= '3') {
                    tileShape.setFillColor(sf::Color(245, 210, 65));
                } else if (tile == 'E') {
                    tileShape.setFillColor(sf::Color(120, 60, 220));
                } else {
                    tileShape.setFillColor(sf::Color(170, 170, 170));
                }

                tileShape.setPosition(sf::Vector2f(static_cast<float>(x * tileSize), static_cast<float>(y * tileSize)));
                window.draw(tileShape);
            }
        }
    }

    void drawPlayer() {
        sf::CircleShape playerShape(static_cast<float>(tileSize) * 0.28F);
        playerShape.setFillColor(sf::Color::Cyan);
        playerShape.setOrigin(playerShape.getGeometricCenter());
        playerShape.setPosition(
            sf::Vector2f(static_cast<float>(player.x * tileSize + tileSize / 2), static_cast<float>(player.y * tileSize + tileSize / 2))
        );
        window.draw(playerShape);
    }

    void drawHud() {
        sf::RectangleShape hudBg(sf::Vector2f(static_cast<float>(windowWidth), static_cast<float>(hudHeight)));
        hudBg.setPosition(sf::Vector2f(0.F, static_cast<float>(mapSize * tileSize)));
        hudBg.setFillColor(sf::Color(20, 20, 28));
        window.draw(hudBg);

        if (!fontLoaded) {
            return;
        }

        sf::Text text(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        text.setPosition(sf::Vector2f(16.F, static_cast<float>(mapSize * tileSize + 16)));
        text.setString(
            "Ключі: " + std::to_string(score) + "/3   |   "
            "WASD/Стрілки - рух   |   ESC - вихід"
        );
        window.draw(text);

        if (score < 3) {
            text.setCharacterSize(20);
            text.setPosition(sf::Vector2f(16.F, static_cast<float>(mapSize * tileSize + 56)));
            text.setString("Збери 3 фрагменти ключа, щоб відчинити двері D");
            window.draw(text);
        } else {
            text.setCharacterSize(20);
            text.setPosition(sf::Vector2f(16.F, static_cast<float>(mapSize * tileSize + 56)));
            text.setString("Двері відкриті! Шукай вихід E");
            window.draw(text);
        }
    }

    void drawPortalWinScreen() {
        sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowWidth), static_cast<float>(windowHeight)));
        bg.setFillColor(sf::Color(5, 5, 20));
        window.draw(bg);

        const float t = clock.getElapsedTime().asSeconds();
        const sf::Vector2f center(static_cast<float>(windowWidth) / 2.F, static_cast<float>(windowHeight) / 2.F - 30.F);

        for (int i = 0; i < 7; ++i) {
            float radius = 35.F + i * 28.F + std::sin(t * 2.F + static_cast<float>(i)) * 8.F;
            sf::CircleShape ring(radius);
            ring.setOrigin(ring.getGeometricCenter());
            ring.setPosition(center);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.F);
            ring.setOutlineColor(sf::Color(120 + i * 15, 60 + i * 20, 220 - i * 15));
            window.draw(ring);
        }

        if (!fontLoaded) {
            return;
        }

        sf::Text winTitle(font, "СИСТЕМА ЗЛАМАННЯ. ВИ ВІЛЬНІ", 36);
        winTitle.setStyle(sf::Text::Bold);
        winTitle.setFillColor(sf::Color::White);
        winTitle.setPosition(sf::Vector2f(28.F, static_cast<float>(windowHeight) - 160.F));
        window.draw(winTitle);

        sf::Text tip(font, "Натисни ESC, щоб закрити гру", 24);
        tip.setFillColor(sf::Color(220, 220, 230));
        tip.setPosition(sf::Vector2f(150.F, static_cast<float>(windowHeight) - 110.F));
        window.draw(tip);
    }

    void render() {
        window.clear();

        if (gameWon) {
            drawPortalWinScreen();
        } else {
            drawMap();
            drawPlayer();
            drawHud();
        }

        window.display();
    }

    sf::Clock clock;
};

int main() {
    LabyrinthGame game;
    game.run();
    return 0;
}