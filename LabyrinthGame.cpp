#include "LabyrinthGame.hpp"
#include "LabyrinthGameShared.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <cstring>

using namespace LabyrinthGameShared;


LabyrinthGame::LabyrinthGame()
    : window(sf::VideoMode({screenWidth, screenHeight}), "Labyrinth of Distorted Reality - First Person (SFML)"),
        handsSprite(handsTexture),        // Прив'язуємо відразу
        screamerSprite(screamerTexture)   // Прив'язуємо відразу (для SFML 3.0)
{
    window.setFramerateLimit(60);

    // 1. Будуємо світ
    buildLargeMap();
    placeStartingItems();
    placeKeysRandomly();
    spawnEnemiesRandomly();

    // 2. Шрифт (із fallback як у меню)
    constexpr std::array<const char*, 12> fontCandidates = {
        "assets/fonts/NotoSans-Regular.ttf",
        "assets/fonts/DejaVuSans.ttf",
        "arial.ttf",
        "assets/arial.ttf",
        "assets/fonts/arial.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansDisplay-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    for (const char* path : fontCandidates) {
        if (!std::filesystem::exists(path)) continue;
        if (font.openFromFile(path)) {
            fontLoaded = true;
        }
    }
    if (!fontLoaded) std::cerr << "[Попередження] Не знайдено шрифт для кнопок/тексту.\n";

    // 3. ЗАВАНТАЖЕННЯ РУК (динамічна зміна залежно від предметів)
    refreshHandsTexture();
    flashlightBeamStrength = 0.0F;
    flashlightStableTimer = 1.4F;
    flashlightBlinkTimer = 0.0F;

    // 4. ЗАВАНТАЖЕННЯ СКРІМЕРА
    if (screamerTexture.loadFromFile("assets/screamer.png")) {
        screamerTexture.setSmooth(true);
        // Центруємо скрімер
        sf::Vector2u size = screamerTexture.getSize();
        screamerSprite.setOrigin({ size.x / 2.0f, size.y / 2.0f });
        screamerSprite.setPosition({ screenWidth / 2.0f, screenHeight / 2.0f });
        screamerLoaded = true;
        std::cerr << "[INFO] Screamer texture loaded.\n";
    } else {
        std::cerr << "[WARNING] assets/screamer.png не знайдено.\n";
    }

    // 5. Інші ресурси та таймери
    loadEnemySpriteAssets();
    loadItemTextures();
    loadSounds();
    initEndButtons();
    resetScreamerTimer(); // Обнуляємо таймер (30 сек)
    transitionState = TransitionState::FadeIn;
    transitionAlpha = 255.0F;
}



void LabyrinthGame::centerEndButtonLabel(EndButton& button, const std::string& text,
                                         float centerX, float centerY, unsigned int size) {
    if (!fontLoaded) return;
    button.label.emplace(font, utf8(text), size);
    button.label->setStyle(sf::Text::Bold);
    button.label->setFillColor(sf::Color(245, 245, 250));
    const auto bounds = button.label->getLocalBounds();
    button.label->setOrigin({bounds.size.x / 2.F, bounds.size.y / 2.F});
    button.label->setPosition({centerX, centerY - 6.F});
}

void LabyrinthGame::initEndButtons() {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);

    restartButton.box.setSize({320.F, 62.F});
    restartButton.box.setOrigin({160.F, 31.F});
    restartButton.box.setPosition({sw / 2.F, sh / 2.F + 120.F});
    restartButton.box.setFillColor(sf::Color(78, 18, 18, 240));
    restartButton.box.setOutlineThickness(3.F);
    restartButton.box.setOutlineColor(sf::Color(220, 70, 70, 255));

    menuButton.box.setSize({320.F, 62.F});
    menuButton.box.setOrigin({160.F, 31.F});
    menuButton.box.setPosition({sw / 2.F, sh / 2.F + 200.F});
    menuButton.box.setFillColor(sf::Color(20, 24, 34, 240));
    menuButton.box.setOutlineThickness(3.F);
    menuButton.box.setOutlineColor(sf::Color(135, 150, 195, 255));

    centerEndButtonLabel(restartButton, "ПОЧАТИ ЗНОВУ", sw / 2.F, sh / 2.F + 120.F, 27);
    centerEndButtonLabel(menuButton, "ПОВЕРНУТИСЬ ДО МЕНЮ", sw / 2.F, sh / 2.F + 200.F, 24);

}

void LabyrinthGame::resetGameState() {
    buildLargeMap();
    placeStartingItems();
    placeKeysRandomly();
    spawnEnemiesRandomly();

    player = sf::Vector2f(1.5F, 1.5F);
    playerAngle = 0.0F;
    walkWavePhase = 0.0F;
    idleSwayPhase = 0.0F;
    cameraBobOffset = 0.0F;
    hp = 100.0F;
    stamina = 100.0F;
    flightVisualTimer = 0.0F;
    sprintVisualTimer = 0.0F;
    handSwayPhase = 0.0F;
    handBobY = 0.0F;
    handBobX = 0.0F;
    handIdlePhase = 0.0F;

    score = 0;
    exitSpawned = false;
    gameWon = false;
    gameover = false;
    showFullMap = false;

    activeEndScreen = EndScreen::None;
    pendingEndScreen = EndScreen::None;
    transitionState = TransitionState::FadeIn;
    transitionAlpha = 255.0F;

    darknessFlicker = 0.0F;
    flickerPhase = 0.0F;
    ambientDarknessAlpha = 0.0F;
    pickupTransitionTimer = 0.0F;
    interactionPressed = false;
    attackPressed = false;
    hasFlashlight = false;
    hasKnife = false;

    refreshHandsTexture();

    screamerActive = false;
    screamerShowTimer = 0.0F;
    resetScreamerTimer();

    if (footstepSound.has_value()) footstepSound->stop();
    footstepTimer = 0.0F;
    portalClock.restart();
}

void LabyrinthGame::updateTransition(float dt) {
    if (transitionState == TransitionState::Idle) return;

    if (transitionState == TransitionState::FadeOut) {
        transitionAlpha += transitionSpeed * dt;
        if (transitionAlpha >= 255.0F) {
            transitionAlpha = 255.0F;
            activeEndScreen = pendingEndScreen;
            transitionState = TransitionState::FadeIn;
            if (activeEndScreen != EndScreen::None) {
                gameover = (activeEndScreen == EndScreen::GameOver);
                gameWon = (activeEndScreen == EndScreen::Victory);
            }
        }
    } else if (transitionState == TransitionState::FadeIn) {
        transitionAlpha -= transitionSpeed * dt;
        if (transitionAlpha <= 0.0F) {
            transitionAlpha = 0.0F;
            transitionState = TransitionState::Idle;
        }
    }
}

// =====================================================================
//  ЗВУК
// =====================================================================
void LabyrinthGame::loadSounds() {
    if (screamerSoundBuffer.loadFromFile("assets/sounds/screamer.wav")) {
        screamerSound.emplace(screamerSoundBuffer);
        screamerSound->setVolume(95.f);
    } else {
        std::cerr << "[Звук] Не знайдено assets/sounds/screamer.wav\n";
    }
    if (footstepSoundBuffer.loadFromFile("assets/sounds/footstep.wav")) {
        footstepSound.emplace(footstepSoundBuffer);
        footstepSound->setVolume(55.f);
    } else {
        std::cerr << "[Звук] Не знайдено assets/sounds/footstep.wav\n";
    }
    if (pickupSoundBuffer.loadFromFile("assets/sounds/pickup.wav")) {
        pickupSound.emplace(pickupSoundBuffer);
        pickupSound->setVolume(80.f);
    } else {
        std::cerr << "[Звук] Не знайдено assets/sounds/pickup.wav\n";
    }
        if (attackSoundBuffer.loadFromFile("assets/sounds/attack.wav")) {
        attackSound.emplace(attackSoundBuffer);
        attackSound->setVolume(85.f);
    }
    if (enemyDeathSoundBuffer.loadFromFile("assets/sounds/enemy_death.wav")) {
        enemyDeathSound.emplace(enemyDeathSoundBuffer);
        enemyDeathSound->setVolume(90.f);
    }
    constexpr std::array<const char*, 4> gameMusicCandidates = {
        "assets/sounds/game_music.ogg",
        "assets/sounds/game_music.mp3",
        "assets/music/game_music.ogg",
        "assets/music/game_music.mp3"
    };
    for (const char* path : gameMusicCandidates) {
        if (!std::filesystem::exists(path)) continue;
        gameMusic.emplace();
        if (gameMusic->openFromFile(path)) {
            gameMusic->setLooping(true);
            gameMusic->setVolume(32.f);
            gameMusic->play();
            break;
        }
        gameMusic.reset();
    }
}
void LabyrinthGame::loadItemTextures() {
    if (flashlightItemTexture.loadFromFile("assets/flashlight_item.png") ||
        flashlightItemTexture.loadFromFile("assets/flashlight_item.bmp")) {
        flashlightItemTexture.setSmooth(true);
        flashlightItemLoaded = true;
    } else {
        std::cerr << "[INFO] Не знайдено flashlight_item (png/bmp), буде fallback.\n";
    }

    if (knifeItemTexture.loadFromFile("assets/knife_item.png") ||
        knifeItemTexture.loadFromFile("assets/knife_item.bmp")) {
        knifeItemTexture.setSmooth(true);
        knifeItemLoaded = true;
    } else {
        std::cerr << "[INFO] Не знайдено knife_item (png/bmp), буде fallback.\n";
    }
}

void LabyrinthGame::updateFootsteps(float dt, bool isWalking, bool isSprinting) {
    if (!isWalking) {
        footstepTimer = 0.0F;
        if (footstepSound.has_value() &&
            footstepSound->getStatus() == sf::Sound::Status::Playing)
            footstepSound->stop();
        return;
    }
    const float interval = isSprinting ? footstepInterval * 0.62F : footstepInterval;
    footstepTimer += dt;
    if (footstepTimer >= interval) {
        footstepTimer = 0.0F;
        if (footstepSound.has_value()) {
            footstepSound->stop();
            footstepSound->play();
        }
    }
}

// =====================================================================
//  СКРІМЕР
// =====================================================================
void LabyrinthGame::resetScreamerTimer() {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(15.0F, 60.0F);
    screamerNextTrigger          = dist(rng);
    screamerTimeSinceLastTrigger = 0.0F;
}

void LabyrinthGame::updateScreamer(float dt) {
    if (screamerActive) {
        screamerShowTimer += dt;
        if (screamerShowTimer >= screamerShowDuration) {
            screamerActive = false;
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> dist(30.0F, 90.0F);
            screamerNextTrigger          = dist(rng);
            screamerTimeSinceLastTrigger = 0.0F;
        }
        return;
    }
    screamerTimeSinceLastTrigger += dt;
    if (screamerTimeSinceLastTrigger >= screamerNextTrigger) {
        screamerActive    = true;
        screamerShowTimer = 0.0F;
        if (screamerSound.has_value()) screamerSound->play();
    }
}


void LabyrinthGame::run() {
    while (window.isOpen()) {
        const float dt = deltaClock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

// =====================================================================
//  УТИЛІТИ КАРТИ
// =====================================================================

void LabyrinthGame::processEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) { window.close(); return; }
        if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape) window.close();
            if (kp->code == sf::Keyboard::Key::M && activeEndScreen == EndScreen::None) showFullMap = !showFullMap;
            if (kp->code == sf::Keyboard::Key::E && activeEndScreen == EndScreen::None) interactionPressed = true;
        }
        if (const auto* mbe = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mbe->button == sf::Mouse::Button::Left && activeEndScreen == EndScreen::None) {
                attackPressed = true;
            }
        }

        if (activeEndScreen != EndScreen::None && transitionState == TransitionState::Idle) {
            if (const auto* mbe = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mbe->button == sf::Mouse::Button::Left) {
                    const sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (restartButton.box.getGlobalBounds().contains(mPos)) {
                        resetGameState();
                        return;
                    }
                    if (menuButton.box.getGlobalBounds().contains(mPos)) {
                        window.close();
                        return;
                    }
                }
            }
        }
    }
}

// =====================================================================
//  UPDATE
// =====================================================================
void LabyrinthGame::update(float dt) {
    updateTransition(dt);

    if (activeEndScreen != EndScreen::None ||
        transitionState == TransitionState::FadeOut) return;

    updateScreamer(dt);
    handleInteraction();
    handleCombat();

    if (hasFlashlight) {
        static thread_local std::mt19937 rng(std::random_device{}());
        flashlightStableTimer -= dt;
        if (flashlightBlinkTimer > 0.0F) {
            flashlightBlinkTimer = std::max(0.0F, flashlightBlinkTimer - dt);
        } else if (flashlightStableTimer <= 0.0F) {
            std::uniform_real_distribution<float> blinkDur(0.04F, 0.16F);
            std::uniform_real_distribution<float> nextStable(1.1F, 3.2F);
            flashlightBlinkTimer = blinkDur(rng);
            flashlightStableTimer = nextStable(rng);
        }

        const float flickerWave = std::sin(flickerPhase * 21.0F) * 0.22F
                                + std::sin(flickerPhase * 33.0F) * 0.12F;
        const float blinkCut = (flashlightBlinkTimer > 0.0F) ? 0.08F : 1.0F;
        flashlightBeamStrength = std::clamp((0.92F + flickerWave) * blinkCut, 0.0F, 1.0F);
    } else {
        flashlightBeamStrength = 0.0F;
        flashlightStableTimer = 0.7F;
        flashlightBlinkTimer = 0.0F;
    }

    if (pickupTransitionTimer > 0.0F) {
        pickupTransitionTimer = std::max(0.0F, pickupTransitionTimer - dt);
        updateEnemies(dt * 0.2F);
        return;
    }

    const float rotationSpeed    = 2.8F;
    const float baseMoveSpeed    = 2.45F;
    const float sprintMultiplier = 1.45F;
    const float flightMultiplier = 2.1F;
    const float sprintDrain      = 16.0F;
    const float flightDrain      = 45.0F;
    const float staminaRegen     = 20.0F;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        playerAngle -= rotationSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        playerAngle += rotationSpeed * dt;

    sf::Vector2f moveDir{0.F, 0.F};
    const sf::Vector2f forward{std::cos(playerAngle), std::sin(playerAngle)};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))   moveDir += forward;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) moveDir -= forward;

    const bool isWalking    = (moveDir.x != 0.F || moveDir.y != 0.F);
    const bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                           || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    const bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    bool  isSprinting = false, isFlightJump = false;
    float moveSpeed   = baseMoveSpeed;

    if (shiftPressed && stamina > 0.05F && isWalking) {
        isSprinting = true; moveSpeed *= sprintMultiplier;
        stamina -= sprintDrain * dt;
        sprintVisualTimer = std::min(1.0F, sprintVisualTimer + dt * 2.8F);
    } else {
        sprintVisualTimer = std::max(0.0F, sprintVisualTimer - dt * 2.2F);
    }
    if (shiftPressed && spacePressed && stamina > 0.05F) {
        isFlightJump = true; moveSpeed *= flightMultiplier;
        stamina -= flightDrain * dt;
        flightVisualTimer = std::min(0.35F, flightVisualTimer + dt * 2.2F);
    } else {
        flightVisualTimer = std::max(0.0F, flightVisualTimer - dt * 2.6F);
    }
    if (!isSprinting && !isFlightJump) stamina += staminaRegen * dt;
    stamina = std::clamp(stamina, 0.0F, 100.0F);

    movePlayer(moveDir, moveSpeed * dt);
    updateFootsteps(dt, isWalking, isSprinting);

    // ===== Хитання камери =====
    if (isWalking) {
        const float walkFreq = 8.6F + sprintVisualTimer * 4.5F + flightVisualTimer * 2.5F;
        const float walkAmp  = 12.5F + sprintVisualTimer * 9.0F + flightVisualTimer * 11.0F;
        walkWavePhase   += dt * walkFreq;
        cameraBobOffset  = std::sin(walkWavePhase) * walkAmp;
        handSwayPhase   += dt * walkFreq;
        handBobY = std::sin(handSwayPhase) * (18.0F + sprintVisualTimer * 14.0F);
        handBobX = std::cos(handSwayPhase * 0.5F) * (7.0F + sprintVisualTimer * 6.0F);
    } else {
        idleSwayPhase   += dt * 1.9F;
        cameraBobOffset  = std::sin(idleSwayPhase) * 2.2F;
        handIdlePhase   += dt * 1.4F;
        handBobY = std::sin(handIdlePhase) * 4.0F;
        handBobX = std::cos(handIdlePhase * 0.6F) * 2.0F;
    }
    if (flightVisualTimer > 0.0F) cameraBobOffset -= 13.0F * flightVisualTimer;

    // Обмежуємо щоб горизонт не виходив за межі — фікс провалювання
    cameraBobOffset = std::clamp(cameraBobOffset,
        -static_cast<float>(screenHeight) * 0.30F,
         static_cast<float>(screenHeight) * 0.30F);

    flickerPhase        += dt * 2.3F;
    darknessFlicker      = std::sin(flickerPhase) * 6.0F + std::sin(flickerPhase * 3.7F) * 4.0F;
    const float healthPenalty = (1.0F - hp / 100.0F) * 60.0F;
    const float baseDarkness = hasFlashlight ? 125.0F : 220.0F;
    ambientDarknessAlpha = std::clamp(baseDarkness + healthPenalty + darknessFlicker,
                                      60.0F, 245.0F);

    revealNearbyKeys();
    collectAtPlayerCell();
    unlockDoorAndSpawnExit();
    updateEnemies(dt);
    checkWin();

    if (hp <= 0.0F && pendingEndScreen == EndScreen::None) {
        pendingEndScreen = EndScreen::GameOver;
        transitionState = TransitionState::FadeOut;
        if (footstepSound.has_value()) footstepSound->stop();
    }
    
    if (gameWon && pendingEndScreen == EndScreen::None) {
        pendingEndScreen = EndScreen::Victory;
        transitionState = TransitionState::FadeOut;
    }
}

// =====================================================================
//  РУХ
// =====================================================================

void LabyrinthGame::render() {
    window.clear(sf::Color(0,0,0));

    if (activeEndScreen == EndScreen::GameOver) {
        drawGameOver();
    } else if (activeEndScreen == EndScreen::Victory) {
        drawVictoryScreen();
    } else {
        const bool isWalking =
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)  ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)  ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
        const bool isSprinting = isWalking &&
            (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
             sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift));

        drawFirstPersonWorld();
        drawPlayerHands(isWalking, isSprinting);
        drawVignette();
        drawMiniMap();
        drawHud();
        if (showFullMap) drawFullMapOverlay();
                if (pickupTransitionTimer > 0.0F) {
            const uint8_t a = static_cast<uint8_t>(std::clamp(
                (pickupTransitionTimer / 0.22F) * 255.0F, 0.0F, 255.0F));
            sf::RectangleShape blink(sf::Vector2f((float)screenWidth, (float)screenHeight));
            blink.setFillColor(sf::Color(0, 0, 0, a));
            window.draw(blink);
        }
        drawScreamer();
    }

    drawTransitionOverlay();
    window.display();
}

