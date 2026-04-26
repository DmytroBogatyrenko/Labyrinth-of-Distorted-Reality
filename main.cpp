#include "Menuscene.hpp"
#include "LabyrinthGame.hpp"

int main() {
    // Одне вікно на весь час роботи програми
    sf::RenderWindow window(
        sf::VideoMode({1100u, 700u}),
        "Labyrinth of Distorted Reality");
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        // ── Меню ────────────────────────────────────────────────────
        MenuScene menu(window);
        const MenuResult menuResult = menu.run();

        if (!window.isOpen() || menuResult == MenuResult::Quit)
            break;

        if (menuResult == MenuResult::StartGame) {
            // ── Гра ─────────────────────────────────────────────────
            // LabyrinthGame сам відкриває своє вікно всередині,
            // але у нас вікно спільне — тому передаємо його.
            // Якщо LabyrinthGame створює своє вікно — просто
            // конструюємо об'єкт і викликаємо run().
            LabyrinthGame game;
            game.run();

            // Після game.run() вікно може бути закрите (ESC).
            // Якщо вікно закрите — виходимо з циклу.
            if (!window.isOpen()) break;

            // Інакше повертаємось до меню (цикл while продовжується).
        }
    }

    return 0;
}