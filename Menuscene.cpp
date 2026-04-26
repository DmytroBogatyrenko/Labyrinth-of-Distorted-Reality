#include "MenuScene.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <random>

// =====================================================================
//  КОНСТРУКТОР
// =====================================================================
MenuScene::MenuScene(sf::RenderWindow& win) : window(win) {
    loadAssets();   
    loadSounds();
    initButtons();  
    initParticles();
    fadeState = FadeState::FadeIn;
    fadeAlpha = 255.0F;
}

// =====================================================================
//  ОСНОВНИЙ ЦИКЛ (ОБРОБКА СЦЕНИ)
// =====================================================================
MenuResult MenuScene::run() {
    sf::Clock localClock;
    MenuResult result = MenuResult::None;

    while (window.isOpen()) {
        float dt = localClock.restart().asSeconds();

        processEvents(result);
        update(dt, result);
        render();

        if (result != MenuResult::None) {
            return result;
        }
    }
    return MenuResult::Quit;
}

// =====================================================================
//  ЗАВАНТАЖЕННЯ РЕСУРСІВ
// =====================================================================
void MenuScene::loadAssets() {
    constexpr std::array<const char*, 6> fontCandidates = {
        "arial.ttf",
        "assets/arial.ttf",
        "assets/fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (const char* path : fontCandidates) {
        if (!std::filesystem::exists(path)) continue;
        if (font.openFromFile(path)) {
            fontLoaded = true;
            return;
        }
    }
    std::cerr << "[Menu] Font not found!\n";

}

void MenuScene::loadSounds() {
    if (std::filesystem::exists("assets/sounds/menu_hover.wav") &&
        hoverBuf.loadFromFile("assets/sounds/menu_hover.wav")) {
        hoverSound.emplace(hoverBuf);
        hoverSound->setVolume(55.f);
    }
    if (std::filesystem::exists("assets/sounds/menu_click.wav") &&
        clickBuf.loadFromFile("assets/sounds/menu_click.wav")) {
        clickSound.emplace(clickBuf);
        clickSound->setVolume(80.f);
    }
}

// =====================================================================
//  ІНІЦІАЛІЗАЦІЯ
// =====================================================================
void MenuScene::initButtons() {
    if (!fontLoaded) return;
    const float cx = static_cast<float>(SW) / 2.0F;

    auto setup = [&](MenuButton& btn, const char* txt, float y, unsigned int sz) {
        btn.box.setSize({300.F, 58.F});
        btn.box.setOrigin({150.F, 29.F});
        btn.box.setPosition({cx, y});
        btn.box.setFillColor(sf::Color(6, 6, 8, 200));
        btn.box.setOutlineThickness(1.5F);
        btn.box.setOutlineColor(sf::Color(70, 70, 80, 175));

        btn.label.emplace(font, sf::String(txt), sz);
        btn.label->setLetterSpacing(2.2F);
        centerText(*btn.label, cx, y);
    };

    setup(btnPlay,  "UVIYTY U TEMRYAVU",  390.F, 23);
    setup(btnRules, "PRAVYLA / SYUZHET",  470.F, 23);
    setup(btnBack,  "<- POVERNUTYS",      625.F, 21);
}

void MenuScene::initParticles() {
    static std::mt19937 rng(std::random_device{}());
    particles.resize(55);
    for (auto& p : particles) {
        std::uniform_real_distribution<float> rx(0.F, (float)SW);
        std::uniform_real_distribution<float> ry(0.F, (float)SH);
        p.pos     = {rx(rng), ry(rng)};
        p.vel     = {std::uniform_real_distribution<float>(-1.5F, 1.5F)(rng), 
                     -std::uniform_real_distribution<float>(15.F, 40.F)(rng)};
        p.alpha   = std::uniform_real_distribution<float>(20.F, 80.F)(rng);
        p.size    = std::uniform_real_distribution<float>(1.F, 3.F)(rng);
        p.maxLife = std::uniform_real_distribution<float>(2.F, 5.F)(rng);
        p.life    = std::uniform_real_distribution<float>(0.F, p.maxLife)(rng);
    }
}

// =====================================================================
//  ОБРОБКА ПОДІЙ (SFML 3)
// =====================================================================
void MenuScene::processEvents(MenuResult& result) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            result = MenuResult::Quit;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                if (page == MenuPage::Rules) {
                    pendingPage = MenuPage::Main;
                    fadeState = FadeState::FadeOut;
                } else {
                    result = MenuResult::Quit;
                }
            }
        }

        if (const auto* mbe = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mbe->button == sf::Mouse::Button::Left) {
                sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (page == MenuPage::Main) {
                    if (btnPlay.box.getGlobalBounds().contains(mPos)) {
                        if (clickSound) clickSound->play();
                        pendingResult = MenuResult::StartGame;
                        fadeState = FadeState::FadeOut;
                    } else if (btnRules.box.getGlobalBounds().contains(mPos)) {
                        if (clickSound) clickSound->play();
                        pendingPage = MenuPage::Rules;
                        fadeState = FadeState::FadeOut;
                    }
                } else if (page == MenuPage::Rules) {
                    if (btnBack.box.getGlobalBounds().contains(mPos)) {
                        if (clickSound) clickSound->play();
                        pendingPage = MenuPage::Main;
                        fadeState = FadeState::FadeOut;
                    }
                }
            }
        }
    }
}

// =====================================================================
//  ЛОГІКА ОНОВЛЕННЯ
// =====================================================================
void MenuScene::update(float dt, MenuResult& result) {
    bgPhase += dt;
    flickerTime += dt;

    for (auto& p : particles) {
        p.life -= dt;
        p.pos += p.vel * dt;
        if (p.life <= 0.F) {
            p.life = p.maxLife;
            p.pos.y = static_cast<float>(SH) + 5.f;
        }
    }

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (page == MenuPage::Main) {
        updateButton(btnPlay, mPos, dt);
        updateButton(btnRules, mPos, dt);
    } else {
        updateButton(btnBack, mPos, dt);
    }

    if (fadeState == FadeState::FadeIn) {
        fadeAlpha -= fadeSpeed * dt;
        if (fadeAlpha <= 0.F) { fadeAlpha = 0.F; fadeState = FadeState::Idle; }
    } else if (fadeState == FadeState::FadeOut) {
        fadeAlpha += fadeSpeed * dt;
        if (fadeAlpha >= 255.F) {
            fadeAlpha = 255.F;
            if (pendingResult != MenuResult::None) result = pendingResult;
            else { page = pendingPage; fadeState = FadeState::FadeIn; }
        }
    }
}

void MenuScene::updateButton(MenuButton& btn, const sf::Vector2f& mouse, float dt) {
    bool wasHovered = btn.hovered;
    btn.hovered = btn.box.getGlobalBounds().contains(mouse);
    if (btn.hovered) {
        if (!wasHovered && hoverSound) hoverSound->play();
        btn.glowTime += dt * 4.0F;
    } else {
        btn.glowTime = 0.F;
    }
}

// =====================================================================
//  ВІДМАЛЬОВУВАННЯ
// =====================================================================
void MenuScene::render() {
    window.clear(sf::Color(0, 0, 0));
    drawBackground();
    drawParticles();
    if (page == MenuPage::Main) drawMainPage();
    else drawRulesPage();
    drawFadeOverlay();
    window.display();
}

// =====================================================================
//  ФОН (Виправлено narrowing conversion)
// =====================================================================
void MenuScene::drawBackground() {
    const float cx = static_cast<float>(SW) / 2.0F;
    const float cy = static_cast<float>(SH) / 2.0F;

    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(SW), static_cast<float>(SH)));
    bg.setFillColor(sf::Color(3, 2, 4));
    window.draw(bg);

    // Туман
    constexpr int fogN = 40;
    sf::VertexArray fog(sf::PrimitiveType::TriangleFan, fogN + 2);
    fog[0].position = sf::Vector2f(cx, static_cast<float>(SH) + 60.f);
    fog[0].color = sf::Color(60, 5, 5, 60);
    
    for (int i = 0; i <= fogN; ++i) {
        float a = static_cast<float>(i) / static_cast<float>(fogN) * 3.14159f;
        
        // Використовуємо sf::Vector2f(...) замість {...}, щоб уникнути narrowing warnings
        float x = cx + std::cos(a) * 900.f;
        float y = static_cast<float>(SH) + 60.f - std::sin(a) * 400.f;
        
        fog[i+1].position = sf::Vector2f(x, y);
        fog[i+1].color = sf::Color(20, 2, 2, 0);
    }
    window.draw(fog);

    // Радіальна віньєтка
    constexpr int vN = 50;
    sf::VertexArray vig(sf::PrimitiveType::TriangleFan, vN + 2);
    vig[0].position = sf::Vector2f(cx, cy);
    vig[0].color = sf::Color(0, 0, 0, 0);
    
    for (int i = 0; i <= vN; ++i) {
        float a = static_cast<float>(i) / static_cast<float>(vN) * 2.f * 3.14159f;
        
        float vx = cx + std::cos(a) * static_cast<float>(SW) * 0.8f;
        float vy = cy + std::sin(a) * static_cast<float>(SH) * 0.8f;
        
        vig[i+1].position = sf::Vector2f(vx, vy);
        vig[i+1].color = sf::Color(0, 0, 0, 220);
    }
    window.draw(vig);
}

void MenuScene::drawParticles() {
    for (const auto& p : particles) {
        float fade = std::sin(p.life / p.maxLife * 3.14159F);
        sf::CircleShape dot(p.size);
        dot.setPosition(p.pos);
        dot.setFillColor(sf::Color(180, 160, 140, static_cast<uint8_t>(p.alpha * fade)));
        window.draw(dot);
    }
}

void MenuScene::drawMainPage() {
    if (!fontLoaded) return;
    const float cx = static_cast<float>(SW) / 2.0F;

    float f = 0.9F + sin(flickerTime * 2.f) * 0.05f;
    uint8_t ta = static_cast<uint8_t>(std::clamp(f * 255.f, 0.f, 255.f));

    sf::Text title(font, "LABIRYNT", 84);
    title.setStyle(sf::Text::Bold);
    title.setLetterSpacing(7.f);
    title.setFillColor(sf::Color(215, 215, 220, ta));
    centerText(title, cx, 150.f);
    window.draw(title);

    sf::Text sub(font, "SPOTVORENOI REALNOSTI", 24);
    sub.setFillColor(sf::Color(120, 20, 20, ta));
    centerText(sub, cx, 245.f);
    window.draw(sub);

    drawButton(btnPlay);
    drawButton(btnRules);
}

void MenuScene::drawRulesPage() {
    if (!fontLoaded) return;
    const float cx = static_cast<float>(SW) / 2.0F;

    sf::Text title(font, "PRAVYLA TA SYUZHET", 35);
    title.setFillColor(sf::Color(200, 200, 200));
    centerText(title, cx, 60.f);
    window.draw(title);

    // Тут зазвичай йде блок тексту правил... (скорочено для читабельності)
    sf::Text rules(font, "Zbery 3 klyuchi ta znaydy vykhid.\nUnikay temryavy.", 20);
    rules.setFillColor(sf::Color(170, 170, 170));
    centerText(rules, cx, 300.f);
    window.draw(rules);

    drawButton(btnBack);
}

void MenuScene::drawButton(MenuButton& btn) {
    if (!btn.label) return;
    if (btn.hovered) {
        btn.box.setOutlineColor(sf::Color(180, 20, 20));
        btn.label->setFillColor(sf::Color::White);
    } else {
        btn.box.setOutlineColor(sf::Color(70, 70, 80, 150));
        btn.label->setFillColor(sf::Color(160, 160, 160));
    }
    window.draw(btn.box);
    window.draw(*btn.label);
}

void MenuScene::drawFadeOverlay() {
    if (fadeAlpha <= 0.f) return;
    sf::RectangleShape rect(sf::Vector2f((float)SW, (float)SH));
    rect.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(fadeAlpha)));
    window.draw(rect);
}

void MenuScene::centerText(sf::Text& text, float cx, float cy) {
    sf::FloatRect b = text.getLocalBounds();
    text.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y + b.size.y / 2.0f});
    text.setPosition({cx, cy});
}