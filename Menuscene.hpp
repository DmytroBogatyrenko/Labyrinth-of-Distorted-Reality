#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <string>
#include <vector>
#include <cmath>

enum class MenuResult { None, StartGame, Quit };
enum class MenuPage   { Main, Rules };
enum class FadeState  { Idle, FadeOut, FadeIn };

// ── Кнопка ───────────────────────────────────────────────────────────
// label — optional бо sf::Text не має дефолтного ctor у SFML 3
struct MenuButton {
    sf::RectangleShape       box;
    std::optional<sf::Text>  label;   // емплейситься після завантаження шрифту
    bool  hovered  = false;
    float glowTime = 0.0F;
};

// ── Клас меню ────────────────────────────────────────────────────────
class MenuScene {
public:
    explicit MenuScene(sf::RenderWindow& window);
    MenuResult run();

private:
    sf::RenderWindow& window;
    sf::Clock         clock;

    sf::Font font;
    bool     fontLoaded = false;

    MenuPage   page          = MenuPage::Main;
    MenuPage   pendingPage   = MenuPage::Main;
    FadeState  fadeState     = FadeState::FadeIn;
    float      fadeAlpha     = 255.0F;
    float      fadeSpeed     = 270.0F;
    MenuResult pendingResult = MenuResult::None;

    float bgPhase     = 0.0F;
    float flickerTime = 0.0F;

    MenuButton btnPlay;
    MenuButton btnRules;
    MenuButton btnBack;

    sf::SoundBuffer          hoverBuf;
    sf::SoundBuffer          clickBuf;
    std::optional<sf::Sound> hoverSound;
    std::optional<sf::Sound> clickSound;
    std::optional<sf::Music> menuMusic;

    struct Particle {
        sf::Vector2f pos, vel;
        float alpha = 0.F, size = 0.F, life = 0.F, maxLife = 0.F;
    };
    std::vector<Particle> particles;

    // Завантажує шрифти та інші візуальні ресурси меню
    void loadAssets();
    // Завантажує звуки кнопок і фонову музику
    void loadSounds();
    // Створює та налаштовує кнопки меню
    void initButtons();
    // Ініціалізує декоративні частинки фону
    void initParticles();

    // Обробляє події (клавіатура/миша/закриття)
    void processEvents(MenuResult& result);
    // Оновлює стани анімацій, hover та fade
    void update(float dt, MenuResult& result);
    // Відмальовує кадр меню
    void render();

    // Малює анімований фон
    void drawBackground();
    // Малює частинки
    void drawParticles();
    // Малює головну сторінку меню
    void drawMainPage();
    // Малює сторінку правил/сюжету
    void drawRulesPage();
    // Малює fade-оверлей при переходах
    void drawFadeOverlay();

    // Оновлює стан однієї кнопки (hover/glow)
    void updateButton(MenuButton& btn, const sf::Vector2f& mouse, float dt);
    // Малює одну кнопку
    void drawButton(MenuButton& btn);
    // Центрує текст відносно точки
    void centerText(sf::Text& text, float cx, float cy);

    static constexpr unsigned int SW = 1100;
    static constexpr unsigned int SH = 700;
};