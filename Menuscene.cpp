#include "MenuScene.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

// =====================================================================
//  КОНСТРУКТОР
// =====================================================================
MenuScene::MenuScene(sf::RenderWindow& win) : window(win) {
    loadAssets();   // спочатку шрифт
    loadSounds();
    initButtons();  // потім кнопки (потребують шрифт)
    initParticles();
    fadeState = FadeState::FadeIn;
    fadeAlpha = 255.0F;
}

// =====================================================================
//  ШРИФТ
// =====================================================================
void MenuScene::loadAssets() {
    fontLoaded = font.openFromFile("arial.ttf");
    if (!fontLoaded)
        std::cerr << "[Menu] arial.ttf not found.\n";
}

// =====================================================================
//  ЗВУК
// =====================================================================
void MenuScene::loadSounds() {
    if (hoverBuf.loadFromFile("assets/sounds/menu_hover.wav")) {
        hoverSound.emplace(hoverBuf);
        hoverSound->setVolume(55.f);
    }
    if (clickBuf.loadFromFile("assets/sounds/menu_click.wav")) {
        clickSound.emplace(clickBuf);
        clickSound->setVolume(80.f);
    }
}

// =====================================================================
//  КНОПКИ
//  Використовуємо optional::emplace — конструює sf::Text з font на місці
// =====================================================================
void MenuScene::initButtons() {
    if (!fontLoaded) return;

    const float cx = static_cast<float>(SW) / 2.0F;

    auto setup = [&](MenuButton& btn, const char* txt,
                     float y, unsigned int sz) {
        // box
        btn.box.setSize({300.F, 58.F});
        btn.box.setOrigin({150.F, 29.F});
        btn.box.setPosition({cx, y});
        btn.box.setFillColor(sf::Color(6, 6, 8, 200));
        btn.box.setOutlineThickness(1.5F);
        btn.box.setOutlineColor(sf::Color(70, 70, 80, 175));

        // label — emplace конструює sf::Text(font, string, size) в пам'яті optional
        btn.label.emplace(font, sf::String(txt), sz);
        btn.label->setLetterSpacing(2.2F);
        centerText(*btn.label, cx, y);
    };

    setup(btnPlay,  "UVIYTY U TEMRYAVU",  390.F, 23);
    setup(btnRules, "PRAVYLA / SYUZHET",  470.F, 23);
    setup(btnBack,  "<- POVERNUTYS",      625.F, 21);
}

// =====================================================================
//  ЧАСТИНКИ
// =====================================================================
void MenuScene::initParticles() {
    static std::mt19937 rng(std::random_device{}());
    particles.resize(55);
    for (auto& p : particles) {
        std::uniform_real_distribution<float> rx(0.F, (float)SW);
        std::uniform_real_distribution<float> ry(0.F, (float)SH);
        std::uniform_real_distribution<float> rv(-5.F, 5.F);
        std::uniform_real_distribution<float> ra(15.F, 70.F);
        std::uniform_real_distribution<float> rs(1.0F, 3.0F);
        std::uniform_real_distribution<float> rl(5.F, 13.F);
        p.pos     = {rx(rng), ry(rng)};
        p.vel     = {rv(rng) * 0.3F,
                     -std::uniform_real_distribution<float>(3.F, 10.F)(rng)};
        p.alpha   = ra(rng);
        p.size    = rs(rng);
        p.maxLife = rl(rng);
        p.life    = p.maxLife *
                    std::uniform_real_distribution<float>(0.F, 1.F)(rng);
    }
}

// =====================================================================
//  ГОЛОВНИЙ ЦИКЛ
// =====================================================================
MenuResult MenuScene::run() {
    MenuResult result = MenuResult::None;
    while (window.isOpen() && result == MenuResult::None) {
        const float dt = std::min(clock.restart().asSeconds(), 0.05F);
        processEvents(result);
        if (result != MenuResult::None) break;
        update(dt, result);
        render();
    }
    return result;
}

// =====================================================================
//  ПОДІЇ
// =====================================================================
void MenuScene::processEvents(MenuResult& result) {
    const sf::Vector2f mouse =
        window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Звук наведення — перевіряємо кожен кадр до pollEvent
    if (fadeState == FadeState::Idle) {
        if (page == MenuPage::Main) {
            const bool np = btnPlay.box.getGlobalBounds().contains(mouse);
            const bool nr = btnRules.box.getGlobalBounds().contains(mouse);
            if ((!btnPlay.hovered && np) || (!btnRules.hovered && nr)) {
                if (hoverSound.has_value() &&
                    hoverSound->getStatus() != sf::Sound::Status::Playing)
                    hoverSound->play();
            }
        } else {
            const bool nb = btnBack.box.getGlobalBounds().contains(mouse);
            if (!btnBack.hovered && nb) {
                if (hoverSound.has_value() &&
                    hoverSound->getStatus() != sf::Sound::Status::Playing)
                    hoverSound->play();
            }
        }
    }

    while (const auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            result = MenuResult::Quit;
            window.close();
            return;
        }

        if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape &&
                fadeState == FadeState::Idle) {
                if (page == MenuPage::Rules) {
                    if (clickSound.has_value()) clickSound->play();
                    pendingPage   = MenuPage::Main;
                    pendingResult = MenuResult::None;
                    fadeState     = FadeState::FadeOut;
                    fadeAlpha     = 0.0F;
                } else {
                    result = MenuResult::Quit;
                    window.close();
                }
                return;
            }
        }

        if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mb->button == sf::Mouse::Button::Left &&
                fadeState == FadeState::Idle) {

                if (page == MenuPage::Main) {
                    if (btnPlay.box.getGlobalBounds().contains(mouse)) {
                        if (clickSound.has_value()) clickSound->play();
                        pendingPage   = MenuPage::Main;
                        pendingResult = MenuResult::StartGame;
                        fadeState     = FadeState::FadeOut;
                        fadeAlpha     = 0.0F;
                    } else if (btnRules.box.getGlobalBounds().contains(mouse)) {
                        if (clickSound.has_value()) clickSound->play();
                        pendingPage   = MenuPage::Rules;
                        pendingResult = MenuResult::None;
                        fadeState     = FadeState::FadeOut;
                        fadeAlpha     = 0.0F;
                    }
                } else {
                    if (btnBack.box.getGlobalBounds().contains(mouse)) {
                        if (clickSound.has_value()) clickSound->play();
                        pendingPage   = MenuPage::Main;
                        pendingResult = MenuResult::None;
                        fadeState     = FadeState::FadeOut;
                        fadeAlpha     = 0.0F;
                    }
                }
            }
        }
    }
}

// =====================================================================
//  UPDATE
// =====================================================================
void MenuScene::update(float dt, MenuResult& result) {
    bgPhase     += dt * 0.32F;
    flickerTime += dt * 2.6F;

    // Частинки
    static std::mt19937 rng(std::random_device{}());
    for (auto& p : particles) {
        p.life += dt;
        p.pos  += p.vel * dt;
        if (p.life >= p.maxLife || p.pos.y < -10.F) {
            std::uniform_real_distribution<float> rx(0.F, (float)SW);
            std::uniform_real_distribution<float> rv(-4.F, 4.F);
            std::uniform_real_distribution<float> ra(12.F, 65.F);
            std::uniform_real_distribution<float> rs(1.0F, 3.0F);
            std::uniform_real_distribution<float> rl(5.F, 14.F);
            p.pos     = {rx(rng), (float)SH + 5.F};
            p.vel     = {rv(rng) * 0.25F,
                         -std::uniform_real_distribution<float>(3.F, 11.F)(rng)};
            p.alpha   = ra(rng);
            p.size    = rs(rng);
            p.maxLife = rl(rng);
            p.life    = 0.F;
        }
    }

    // Hover кнопок
    const sf::Vector2f mouse =
        window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (page == MenuPage::Main) {
        updateButton(btnPlay,  mouse, dt);
        updateButton(btnRules, mouse, dt);
    } else {
        updateButton(btnBack, mouse, dt);
    }

    // Fade логіка
    if (fadeState == FadeState::FadeIn) {
        fadeAlpha -= fadeSpeed * dt;
        if (fadeAlpha <= 0.0F) {
            fadeAlpha = 0.0F;
            fadeState = FadeState::Idle;
        }
    } else if (fadeState == FadeState::FadeOut) {
        fadeAlpha += fadeSpeed * dt;
        if (fadeAlpha >= 255.0F) {
            fadeAlpha = 255.0F;
            if (pendingResult == MenuResult::StartGame) {
                result = MenuResult::StartGame;
                return;
            }
            page      = pendingPage;
            fadeState = FadeState::FadeIn;
        }
    }
}

// =====================================================================
//  RENDER
// =====================================================================
void MenuScene::render() {
    window.clear(sf::Color(0, 0, 0));
    drawBackground();
    drawParticles();
    if (page == MenuPage::Main) drawMainPage();
    else                         drawRulesPage();
    drawFadeOverlay();
    window.display();
}

// =====================================================================
//  ФОН
// =====================================================================
void MenuScene::drawBackground() {
    const float cx = (float)SW / 2.0F;

    // Чорний фон
    sf::RectangleShape bg(sf::Vector2f((float)SW, (float)SH));
    bg.setFillColor(sf::Color(3, 2, 4));
    window.draw(bg);

    // Червоний туман знизу
    constexpr int fogN = 48;
    sf::VertexArray fog(sf::PrimitiveType::TriangleFan, fogN + 2);
    fog[0].position = {cx, (float)SH + 80.F};
    fog[0].color    = sf::Color(55, 5, 5, 50);
    for (int i = 0; i <= fogN; ++i) {
        const float a = (float)i / fogN * 3.14159F;
        fog[i+1].position = {cx + std::cos(a) * 920.F,
                              (float)SH + 80.F - std::sin(a) * 440.F};
        fog[i+1].color = sf::Color(20, 2, 2, 0);
    }
    window.draw(fog);

    // Пульсуючий обрис
    const float beat = std::abs(std::sin(bgPhase * 1.1F));
    sf::CircleShape pulse(160.F + beat * 20.F);
    pulse.setOrigin({pulse.getRadius(), pulse.getRadius()});
    pulse.setPosition({cx, (float)SH * 0.5F});
    pulse.setFillColor(sf::Color::Transparent);
    pulse.setOutlineThickness(2.0F);
    pulse.setOutlineColor(sf::Color(60, 3, 3,
        static_cast<uint8_t>(beat * 35.F)));
    window.draw(pulse);

    // Радіальна темна віньєтка
    constexpr int vN = 64;
    sf::VertexArray vig(sf::PrimitiveType::TriangleFan, vN + 2);
    vig[0].position = {cx, (float)SH / 2.F};
    vig[0].color    = sf::Color(0, 0, 0, 0);
    for (int i = 0; i <= vN; ++i) {
        const float a = (float)i / vN * 2.0F * 3.14159F;
        vig[i+1].position = {cx + std::cos(a) * (float)SW * 0.70F,
                              (float)SH / 2.F + std::sin(a) * (float)SH * 0.70F};
        vig[i+1].color = sf::Color(0, 0, 0, 215);
    }
    window.draw(vig);
}

// =====================================================================
//  ЧАСТИНКИ
// =====================================================================
void MenuScene::drawParticles() {
    for (const auto& p : particles) {
        const float fade = std::sin(p.life / p.maxLife * 3.14159F);
        const uint8_t a  = static_cast<uint8_t>(p.alpha * fade);
        sf::CircleShape dot(p.size);
        dot.setOrigin({p.size, p.size});
        dot.setPosition(p.pos);
        dot.setFillColor(sf::Color(175, 155, 135, a));
        window.draw(dot);
    }
}

// =====================================================================
//  ГОЛОВНА СТОРІНКА
// =====================================================================
void MenuScene::drawMainPage() {
    if (!fontLoaded) return;
    const float cx = (float)SW / 2.0F;

    // Мерехтіння назви
    const float f = 0.88F
        + std::sin(flickerTime * 1.2F) * 0.055F
        + std::sin(flickerTime * 3.5F) * 0.028F
        + std::sin(flickerTime * 8.3F) * 0.012F;
    const uint8_t ta = static_cast<uint8_t>(
        std::clamp(f * 255.F, 0.F, 255.F));

    // Тінь
    {
        sf::Text shadow(font, sf::String("LABIRYNT"), 84);
        shadow.setStyle(sf::Text::Bold);
        shadow.setLetterSpacing(7.0F);
        shadow.setFillColor(sf::Color(70, 0, 0, ta / 4));
        centerText(shadow, cx + 4.F, 154.F);
        window.draw(shadow);
    }
    // Назва
    {
        sf::Text title(font, sf::String("LABIRYNT"), 84);
        title.setStyle(sf::Text::Bold);
        title.setLetterSpacing(7.0F);
        title.setFillColor(sf::Color(215, 215, 220, ta));
        centerText(title, cx, 150.F);
        window.draw(title);
    }
    // Підзаголовок
    {
        sf::Text sub(font, sf::String("SPOTVORENOI  REALNOSTI"), 25);
        sub.setLetterSpacing(5.0F);
        sub.setFillColor(sf::Color(110, 18, 18, ta));
        centerText(sub, cx, 248.F);
        window.draw(sub);
    }

    // Декоративна лінія
    {
        const float ly = 300.F;
        sf::RectangleShape ll({210.F, 1.F});
        ll.setFillColor(sf::Color(75, 10, 10, 150));
        ll.setPosition({cx - 300.F, ly});
        window.draw(ll);

        sf::RectangleShape dm({9.F, 9.F});
        dm.setOrigin({4.5F, 4.5F});
        dm.setPosition({cx, ly});
        dm.setFillColor(sf::Color(130, 18, 18, 200));
        dm.setRotation(sf::degrees(45.F));
        window.draw(dm);

        sf::RectangleShape lr({210.F, 1.F});
        lr.setFillColor(sf::Color(75, 10, 10, 150));
        lr.setPosition({cx + 90.F, ly});
        window.draw(lr);
    }

    // Цитата
    {
        sf::Text quote(font, sf::String("\"Ne vsi dveri vedut' nazovni...\""), 17);
        quote.setFillColor(sf::Color(80, 75, 72, 170));
        centerText(quote, cx, 326.F);
        window.draw(quote);
    }

    drawButton(btnPlay);
    drawButton(btnRules);

    // ESC підказка
    {
        sf::Text esc(font, sf::String("ESC - exit"), 15);
        esc.setFillColor(sf::Color(48, 48, 52, 145));
        centerText(esc, cx, 660.F);
        window.draw(esc);
    }
}

// =====================================================================
//  СТОРІНКА ПРАВИЛ
// =====================================================================
void MenuScene::drawRulesPage() {
    if (!fontLoaded) return;
    const float cx = (float)SW / 2.0F;

    // Заголовок
    {
        sf::Text title(font, sf::String("PRAVYLA TA SYUZHET"), 38);
        title.setStyle(sf::Text::Bold);
        title.setLetterSpacing(3.5F);
        title.setFillColor(sf::Color(205, 205, 215));
        centerText(title, cx, 55.F);
        window.draw(title);
    }
    // Лінія
    {
        sf::RectangleShape line({580.F, 1.F});
        line.setOrigin({290.F, 0.5F});
        line.setPosition({cx, 102.F});
        line.setFillColor(sf::Color(90, 12, 12, 190));
        window.draw(line);
    }

    // Контент (лише ASCII — без проблем з кодуванням)
    struct RuleLine {
        const char* text;
        sf::Color   color;
        bool        header;
        float       extraY;
    };
    const RuleLine content[] = {
        {"SYUZHET",                                         {155,28,28},   true,  0.F},
        {"",                                                {0,0,0,0},     false, 4.F},
        {"Ty prokydayesh'sya u labirynti.",                 {188,183,178}, false, 0.F},
        {"Temryava zhyva - vona stezhyt' za toboyu.",       {188,183,178}, false, 0.F},
        {"Znaydy try klyuchi, vidimkny vykhid.",            {188,183,178}, false, 0.F},
        {"Ale pospishi - ty tut ne odyn...",                {165,42,42},   false, 0.F},
        {"",                                                {0,0,0,0},     false, 8.F},
        {"YAK HRATY",                                       {155,28,28},   true,  0.F},
        {"",                                                {0,0,0,0},     false, 4.F},
        {"W / S         - rukh vperyod / nazad",            {168,168,174}, false, 0.F},
        {"A / D         - povorot livo / pravo",            {168,168,174}, false, 0.F},
        {"Shift         - bihty (stamina)",                 {168,168,174}, false, 0.F},
        {"Shift+Space   - strybok-ryvok",                   {168,168,174}, false, 0.F},
        {"M             - povna karta",                     {168,168,174}, false, 0.F},
        {"ESC           - vykhid",                          {168,168,174}, false, 0.F},
        {"",                                                {0,0,0,0},     false, 8.F},
        {"Zbery 3 klyuchi -> z'yavyt'sya vykhid (E)",       {195,188,95},  false, 0.F},
        {"Stezhky za HP! 0 HP = kinets'.",                  {195,65,65},   false, 0.F},
    };

    float y        = 120.F;
    const float lx = cx - 285.F;

    for (const auto& l : content) {
        y += l.extraY;
        if (l.text[0] == '\0') { y += 6.F; continue; }

        sf::Text t(font, sf::String(l.text), l.header ? 21U : 18U);
        t.setFillColor(l.color);
        if (l.header) {
            t.setStyle(sf::Text::Bold);
            t.setLetterSpacing(3.0F);
            centerText(t, cx, y);
        } else {
            const auto b = t.getLocalBounds();
            t.setOrigin({0.F, b.size.y / 2.F});
            t.setPosition({lx, y});
        }
        window.draw(t);
        y += l.header ? 28.F : 24.F;
    }

    drawButton(btnBack);
}

// =====================================================================
//  FADE OVERLAY
// =====================================================================
void MenuScene::drawFadeOverlay() {
    if (fadeAlpha <= 0.0F) return;
    sf::RectangleShape ov(sf::Vector2f((float)SW, (float)SH));
    ov.setFillColor(sf::Color(0, 0, 0,
        static_cast<uint8_t>(std::clamp(fadeAlpha, 0.F, 255.F))));
    window.draw(ov);
}

// =====================================================================
//  КНОПКИ
// =====================================================================
void MenuScene::updateButton(MenuButton& btn,
                              const sf::Vector2f& mouse, float dt) {
    btn.hovered = btn.box.getGlobalBounds().contains(mouse);
    if (btn.hovered) btn.glowTime += dt * 4.2F;
    else             btn.glowTime  = 0.0F;
}

void MenuScene::drawButton(MenuButton& btn) {
    if (!btn.label.has_value()) return;

    if (btn.hovered) {
        const float g = std::abs(std::sin(btn.glowTime)) * 0.55F + 0.45F;

        // Зовнішнє свічення
        sf::RectangleShape glow = btn.box;
        glow.setSize(btn.box.getSize() + sf::Vector2f(12.F, 12.F));
        glow.setOrigin(glow.getSize() / 2.0F);
        glow.setPosition(btn.box.getPosition());
        glow.setFillColor(sf::Color::Transparent);
        glow.setOutlineThickness(3.0F);
        glow.setOutlineColor(sf::Color(150, 20, 20,
            static_cast<uint8_t>(g * 240.F)));
        window.draw(glow);

        btn.box.setFillColor(sf::Color(22, 8, 8, 225));
        btn.box.setOutlineColor(sf::Color(185, 30, 30, 240));
        btn.label->setFillColor(sf::Color(235, 215, 215));
    } else {
        btn.box.setFillColor(sf::Color(6, 6, 8, 200));
        btn.box.setOutlineColor(sf::Color(70, 70, 80, 175));
        btn.label->setFillColor(sf::Color(155, 150, 150));
    }

    window.draw(btn.box);
    window.draw(*btn.label);
}

// =====================================================================
//  ЦЕНТРУВАННЯ ТЕКСТУ
// =====================================================================
void MenuScene::centerText(sf::Text& text, float cx, float cy) {
    const auto b = text.getLocalBounds();
    text.setOrigin({b.position.x + b.size.x / 2.0F,
                    b.position.y + b.size.y / 2.0F});
    text.setPosition({cx, cy});
}