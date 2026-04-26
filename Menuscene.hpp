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

    struct Particle {
        sf::Vector2f pos, vel;
        float alpha = 0.F, size = 0.F, life = 0.F, maxLife = 0.F;
    };
    std::vector<Particle> particles;

    void loadAssets();
    void loadSounds();
    void initButtons();
    void initParticles();

    void processEvents(MenuResult& result);
    void update(float dt, MenuResult& result);
    void render();

    void drawBackground();
    void drawParticles();
    void drawMainPage();
    void drawRulesPage();
    void drawFadeOverlay();

    void updateButton(MenuButton& btn, const sf::Vector2f& mouse, float dt);
    void drawButton(MenuButton& btn);
    void centerText(sf::Text& text, float cx, float cy);

    static constexpr unsigned int SW = 1100;
    static constexpr unsigned int SH = 700;
};