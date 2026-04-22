#include "LabyrinthGame.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>

namespace {
enum EnemyState : int {
    EnemyWander = 0,
    EnemyAlert  = 1,
    EnemyChase  = 2,
    EnemySearch = 3,
    EnemyAttack = 4
};
}

// =====================================================================
//  КОНСТРУКТОР
// =====================================================================
LabyrinthGame::LabyrinthGame()
    : window(sf::VideoMode({screenWidth, screenHeight}),
             "Лабіринт Спотвореної Реальності - First Person (SFML)") {
    window.setFramerateLimit(60);

    buildLargeMap();
    placeKeysRandomly();
    spawnEnemiesRandomly();

    fontLoaded = font.openFromFile("arial.ttf");
    if (!fontLoaded)
        std::cerr << "[Попередження] Не знайдено arial.ttf. Текст HUD буде вимкнено.\n";

    loadEnemySpriteAssets();
    loadSounds();

    if (screamerTexture.loadFromFile("assets/screamer.png")) {
        screamerTexture.setSmooth(true);
        screamerLoaded = true;
        std::cerr << "[INFO] Screamer texture loaded.\n";
    } else {
        std::cerr << "[Попередження] assets/screamer.png не знайдено. Fallback-квадрат.\n";
    }

    resetScreamerTimer();
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

void LabyrinthGame::drawScreamer() {
    if (!screamerActive) return;

    const float flyInEnd = 0.18F;
    const float holdEnd  = 0.55F;
    const float totalDur = screamerShowDuration;

    float scale = 1.0F;
    float alpha = 255.0F;

    if (screamerShowTimer < flyInEnd) {
        const float t = screamerShowTimer / flyInEnd;
        scale = 0.05F + t * 0.95F;
    } else if (screamerShowTimer < holdEnd) {
        scale = 1.0F;
    } else {
        const float t = (screamerShowTimer - holdEnd) / (totalDur - holdEnd);
        alpha = 255.0F * (1.0F - t);
    }

    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);

    if (screamerLoaded) {
        sf::Sprite sp(screamerTexture);
        const sf::Vector2u texSize = screamerTexture.getSize();
        if (texSize.x > 0 && texSize.y > 0) {
            const float fitScale = std::max(sw / texSize.x, sh / texSize.y) * scale;
            sp.setOrigin(sf::Vector2f(texSize.x / 2.F, texSize.y / 2.F));
            sp.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F));
            sp.setScale(sf::Vector2f(fitScale, fitScale));
            sp.setColor(sf::Color(255, 255, 255,
                static_cast<uint8_t>(std::clamp(alpha, 0.F, 255.F))));
            window.draw(sp);
        }
    } else {
        sf::RectangleShape rect(sf::Vector2f(sw * scale, sh * scale));
        rect.setOrigin(sf::Vector2f(sw * scale / 2.F, sh * scale / 2.F));
        rect.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F));
        rect.setFillColor(sf::Color(200, 0, 0,
            static_cast<uint8_t>(std::clamp(alpha, 0.F, 255.F))));
        window.draw(rect);
        if (fontLoaded && scale > 0.5F) {
            sf::Text boo(font, "BOO!", 120);
            boo.setStyle(sf::Text::Bold);
            boo.setFillColor(sf::Color(255, 255, 255,
                static_cast<uint8_t>(std::clamp(alpha, 0.F, 255.F))));
            const auto b = boo.getLocalBounds();
            boo.setOrigin(sf::Vector2f(b.size.x / 2.F, b.size.y / 2.F));
            boo.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F));
            window.draw(boo);
        }
    }
}

// =====================================================================
//  ГОЛОВНИЙ ЦИКЛ
// =====================================================================
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
bool LabyrinthGame::isInsideMap(int x, int y) const {
    return x >= 0 && x < mapWidth && y >= 0 && y < mapHeight;
}

char LabyrinthGame::tileAt(int x, int y) const {
    return map[y][x];
}

bool LabyrinthGame::isBlockingTile(char tile) const {
    if (tile == '#') return true;
    if (tile == 'D' && score < 3) return true;
    return false;
}

int LabyrinthGame::countWallNeighbors(int x, int y) const {
    int walls = 0;
    for (int oy = -1; oy <= 1; ++oy)
        for (int ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) continue;
            const int nx = x + ox, ny = y + oy;
            if (!isInsideMap(nx, ny) || map[ny][nx] == '#') ++walls;
        }
    return walls;
}

// =====================================================================
//  ГЕНЕРАЦІЯ КАРТИ
// =====================================================================
void LabyrinthGame::buildLargeMap() {
    const std::vector<std::vector<std::string>> chunks = {
        {
            "################",
            "#......#.......#",
            "#.####.#.#####.#",
            "#.#..#.#.....#.#",
            "#.#..#.###.#.#.#",
            "#....#...#.#...#",
            "#.####.#.#.###.#",
            "#......#.#...#.#",
            "#.######.###.#.#",
            "#.........#.#..#",
            "#.######.#.#.###",
            "#.#....#.#.#...#",
            "#.#.##.#.#.###.#",
            "#...##...#.....#",
            "#..............#",
            "################",
        },
        {
            "################",
            "#....#.........#",
            "###..#.#####.#.#",
            "#....#.....#.#.#",
            "#.######.#.#.#.#",
            "#......#.#.#.#.#",
            "#.####.#.#.#.#.#",
            "#.#....#.#...#.#",
            "#.#.####.#####.#",
            "#.#......#.....#",
            "#.######.#.###.#",
            "#......#.#.#...#",
            "#.####.#.#.#.###",
            "#....#...#.#...#",
            "#....#####.....#",
            "################",
        },
        {
            "################",
            "#..............#",
            "#.############.#",
            "#.#..........#.#",
            "#.#.########.#.#",
            "#.#.#......#.#.#",
            "#...#.##.#.#...#",
            "###.#.#..#.#.###",
            "#...#.#..#.#...#",
            "#.###.####.###.#",
            "#.#..........#.#",
            "#.#.########.#.#",
            "#............#.#",
            "#.############.#",
            "#..............#",
            "################",
        },
        {
            "################",
            "#..#........#..#",
            "#..#.####.#.#..#",
            "#..#.#....#.#..#",
            "#....#....#....#",
            "####.#.##.#.####",
            "#....#....#....#",
            "#.##.######.##.#",
            "#..............#",
            "#.##.######.##.#",
            "#....#....#....#",
            "####.#.##.#.####",
            "#.........#....#",
            "#..#.#....#.#..#",
            "#..#.######.#..#",
            "################",
        },
        {
            "################",
            "#......##......#",
            "#.####.##.####.#",
            "#.#..........#.#",
            "#.#.########.#.#",
            "#.#.#........#.#",
            "#.#.#.####.#.#.#",
            "#...#.#..#.#...#",
            "###.#.#..#.#.###",
            "#...#....#.#...#",
            "#.#.#.####.#.#.#",
            "#.#.#......#.#.#",
            "#.#.########.#.#",
            "#.#..........#.#",
            "#.####.##.####.#",
            "################",
        }
    };

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> pick(0, static_cast<int>(chunks.size()) - 1);

    map.assign(mapHeight, std::string(mapWidth, '#'));
    std::vector<int> prevRow(chunkGrid, -1);

    for (int ty = 0; ty < chunkGrid; ++ty) {
        int prev = -1;
        for (int tx = 0; tx < chunkGrid; ++tx) {
            int idx = pick(rng), guard = 0;
            while ((idx == prev || idx == prevRow[tx]) && guard < 20) {
                idx = pick(rng); ++guard;
            }
            const auto& chunk = chunks[idx];
            for (int y = 0; y < chunkSize; ++y)
                for (int x = 0; x < chunkSize; ++x)
                    map[ty * chunkSize + y][tx * chunkSize + x] = chunk[y][x];
            prev = idx;
            prevRow[tx] = idx;
        }
    }

    for (int c = 1; c < chunkGrid; ++c) {
        const int seamXLeft  = c * chunkSize - 1;
        const int seamXRight = c * chunkSize;
        for (int y = 3; y < mapHeight - 3; y += 6)
            for (int k = 0; k < 3 && y + k < mapHeight - 1; ++k) {
                map[y + k][seamXLeft]  = '.';
                map[y + k][seamXRight] = '.';
            }

        const int seamYTop    = c * chunkSize - 1;
        const int seamYBottom = c * chunkSize;
        for (int x = 3; x < mapWidth - 3; x += 6)
            for (int k = 0; k < 3 && x + k < mapWidth - 1; ++k) {
                map[seamYTop][x + k]    = '.';
                map[seamYBottom][x + k] = '.';
            }
    }

    player = sf::Vector2f(1.5F, 1.5F);
    map[1][1] = '.'; map[1][2] = '.'; map[2][1] = '.';

    map[mapHeight - 4][mapWidth - 4] = 'D';
    map[mapHeight - 4][mapWidth - 3] = 'e';
    map[mapHeight - 5][mapWidth - 4] = '.';
    map[mapHeight - 4][mapWidth - 5] = '.';

    for (int x = 1; x <= mapWidth - 5; ++x)  map[2][x] = '.';
    for (int y = 2; y <= mapHeight - 4; ++y)  map[y][mapWidth - 5] = '.';
}

void LabyrinthGame::placeKeysRandomly() {
    keys.clear();
    std::vector<sf::Vector2i> candidates;
    candidates.reserve(mapWidth * mapHeight);

    for (int y = 1; y < mapHeight - 1; ++y)
        for (int x = 1; x < mapWidth - 1; ++x) {
            if (map[y][x] != '.') continue;
            if (x <= 4 && y <= 4) continue;
            if (x >= mapWidth - 7 && y >= mapHeight - 7) continue;
            candidates.push_back(sf::Vector2i{x, y});
        }

    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::vector<sf::Vector2i> picked;
    for (const auto& pos : candidates) {
        bool ok = true;
        for (const auto& p : picked) {
            int dx = p.x - pos.x, dy = p.y - pos.y;
            if (dx * dx + dy * dy < 120) { ok = false; break; }
        }
        if (!ok) continue;
        picked.push_back(pos);
        if (picked.size() == 3) break;
    }

    for (int i = 0; i < static_cast<int>(picked.size()); ++i) {
        const auto pos = picked[i];
        map[pos.y][pos.x] = static_cast<char>('1' + i);
        keys.push_back(KeyInfo{pos, false, false});
    }
}

void LabyrinthGame::spawnEnemiesRandomly() {
    enemies.clear();
    std::vector<sf::Vector2f> candidates;
    candidates.reserve(mapWidth * mapHeight);

    for (int y = 2; y < mapHeight - 2; ++y)
        for (int x = 2; x < mapWidth - 2; ++x) {
            if (map[y][x] != '.') continue;
            if (x < 6 && y < 6) continue;
            candidates.emplace_back(static_cast<float>(x) + 0.5F,
                                    static_cast<float>(y) + 0.5F);
        }

    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    constexpr int enemyCount = 4;
    for (const auto& candidate : candidates) {
        bool ok = true;
        for (const auto& enemy : enemies) {
            const sf::Vector2f d = enemy.position - candidate;
            if (d.x * d.x + d.y * d.y < 70.0F) { ok = false; break; }
        }
        if (!ok) continue;

        EnemyInfo enemy;
        enemy.position    = candidate;
        enemy.speed       = 1.3F;
        enemy.state       = EnemyWander;
        enemy.wanderTarget = candidate;
        enemies.push_back(enemy);

        if (static_cast<int>(enemies.size()) >= enemyCount) break;
    }
}

bool LabyrinthGame::loadEnemyFrameSet(const std::string& patternPrefix, int count,
                                       std::vector<sf::Texture>& outFrames) {
    outFrames.clear();
    outFrames.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        sf::Texture frame;
        if (!frame.loadFromFile(patternPrefix + std::to_string(i) + ".png"))
            return false;
        frame.setSmooth(true);
        outFrames.push_back(std::move(frame));
    }
    return !outFrames.empty();
}

void LabyrinthGame::loadEnemySpriteAssets() {
    const bool walkOk   = loadEnemyFrameSet("assets/npc/walk_",   6, enemyWalkFrames);
    const bool attackOk = loadEnemyFrameSet("assets/npc/attack_", 4, enemyAttackFrames);
    const bool alertOk  = loadEnemyFrameSet("assets/npc/alert_",  3, enemyAlertFrames);

    bool singleModelOk = false;
    if (!walkOk) {
        sf::Texture single;
        if (single.loadFromFile("assets/npc/model.png")) {
            single.setSmooth(true);
            enemyWalkFrames.clear();
            enemyWalkFrames.push_back(std::move(single));
            singleModelOk = true;
        }
    }

    enemySpriteAssetsLoaded = walkOk || singleModelOk;
    if (enemySpriteAssetsLoaded && !attackOk) enemyAttackFrames = enemyWalkFrames;
    if (enemySpriteAssetsLoaded && !alertOk)  enemyAlertFrames  = enemyWalkFrames;

    if (!enemySpriteAssetsLoaded) {
        enemyWalkFrames.clear(); enemyAttackFrames.clear(); enemyAlertFrames.clear();
        std::cerr << "[Попередження] NPC спрайти не знайдені. Використовується fallback-силует.\n";
        if (std::filesystem::exists("assets/npc/model.fbx"))
            std::cerr << "[INFO] Знайдено assets/npc/model.fbx.\n";
    } else {
        std::cerr << "[INFO] NPC спрайти завантажено. walk=" << enemyWalkFrames.size()
                  << ", attack=" << enemyAttackFrames.size()
                  << ", alert=" << enemyAlertFrames.size() << '\n';
    }
}

// =====================================================================
//  ПОДІЇ
// =====================================================================
void LabyrinthGame::processEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }
        if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Escape) window.close();
            if (kp->code == sf::Keyboard::Key::M) showFullMap = !showFullMap;
        }
    }
}

// =====================================================================
//  UPDATE
// =====================================================================
void LabyrinthGame::update(float dt) {
    if (gameWon || gameover) return;

    updateScreamer(dt);

    // ===== Рух гравця =====
    const float rotationSpeed    = 1.8F;
    const float baseMoveSpeed    = 3.0F;
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
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        moveDir += forward;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        moveDir -= forward;

    const bool isWalking    = (moveDir.x != 0.F || moveDir.y != 0.F);
    const bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                           || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    const bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    bool  isSprinting  = false;
    bool  isFlightJump = false;
    float moveSpeed    = baseMoveSpeed;

    if (shiftPressed && stamina > 1.0F && isWalking) {
        isSprinting = true;
        moveSpeed  *= sprintMultiplier;
        stamina    -= sprintDrain * dt;
        sprintVisualTimer = std::min(1.0F, sprintVisualTimer + dt * 2.8F);
    } else {
        sprintVisualTimer = std::max(0.0F, sprintVisualTimer - dt * 2.2F);
    }

    if (shiftPressed && spacePressed && stamina > 1.0F) {
        isFlightJump = true;
        moveSpeed   *= flightMultiplier;
        stamina     -= flightDrain * dt;
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
        const float walkFreq = 11.0F + sprintVisualTimer * 5.0F + flightVisualTimer * 3.0F;
        const float walkAmp  =  9.0F + sprintVisualTimer * 8.0F + flightVisualTimer * 9.0F;
        walkWavePhase   += dt * walkFreq;
        cameraBobOffset  = std::sin(walkWavePhase) * walkAmp;

        handSwayPhase += dt * walkFreq;
        handBobY = std::sin(handSwayPhase) * (18.0F + sprintVisualTimer * 14.0F);
        handBobX = std::cos(handSwayPhase * 0.5F) * (7.0F + sprintVisualTimer * 6.0F);
    } else {
        idleSwayPhase   += dt * 1.9F;
        cameraBobOffset  = std::sin(idleSwayPhase) * 2.2F;

        handIdlePhase += dt * 1.4F;
        handBobY = std::sin(handIdlePhase) * 4.0F;
        handBobX = std::cos(handIdlePhase * 0.6F) * 2.0F;
    }

    if (flightVisualTimer > 0.0F) cameraBobOffset -= 13.0F * flightVisualTimer;

    // ===== Мерехтіння темряви =====
    flickerPhase        += dt * 2.3F;
    darknessFlicker      = std::sin(flickerPhase) * 6.0F + std::sin(flickerPhase * 3.7F) * 4.0F;
    ambientDarknessAlpha = std::clamp(
        (1.0F - hp / 100.0F) * 110.0F + darknessFlicker, 0.0F, 160.0F);

    revealNearbyKeys();
    collectAtPlayerCell();
    unlockDoorAndSpawnExit();
    updateEnemies(dt);
    checkWin();

    // ===== Game Over =====
    if (hp <= 0.0F) {
        gameover = true;
        if (footstepSound.has_value()) footstepSound->stop();
    }
}

// =====================================================================
//  РУХ
// =====================================================================
void LabyrinthGame::movePlayer(const sf::Vector2f& dir, float distanceStep) {
    if (dir.x == 0.F && dir.y == 0.F) return;
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    const sf::Vector2f normalized = dir / len;
    const sf::Vector2f candidate  = player + normalized * distanceStep;
    const int tx = static_cast<int>(candidate.x);
    const int ty = static_cast<int>(candidate.y);
    if (!isInsideMap(tx, ty)) return;
    if (isBlockingTile(tileAt(tx, ty))) return;
    player = candidate;
}

bool LabyrinthGame::hasLineOfSight(const sf::Vector2f& from, const sf::Vector2f& to,
                                    float step) const {
    const sf::Vector2f delta = to - from;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist <= 0.001F) return true;
    const sf::Vector2f dir = delta / dist;
    for (float t = 0.0F; t < dist; t += step) {
        const sf::Vector2f p = from + dir * t;
        const int tx = static_cast<int>(p.x);
        const int ty = static_cast<int>(p.y);
        if (!isInsideMap(tx, ty)) return false;
        if (isBlockingTile(tileAt(tx, ty))) return false;
    }
    return true;
}

bool LabyrinthGame::isWalkableEnemyCell(int x, int y) const {
    if (!isInsideMap(x, y)) return false;
    const char tile = tileAt(x, y);
    return tile == '.' || tile == 'E' || (tile >= '1' && tile <= '3');
}

void LabyrinthGame::moveEnemyToward(EnemyInfo& enemy, const sf::Vector2f& target,
                                     float dt, float speedScale) {
    sf::Vector2f delta = target - enemy.position;
    const float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (len <= 0.001F) return;
    const sf::Vector2f dir  = delta / len;
    sf::Vector2f candidate  = enemy.position + dir * enemy.speed * speedScale * dt;
    if (isWalkableEnemyCell(static_cast<int>(candidate.x),
                             static_cast<int>(candidate.y)))
        enemy.position = candidate;
}

sf::Vector2f LabyrinthGame::chooseEnemyWanderTarget(const sf::Vector2f& origin) const {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> offset(-6, 6);
    for (int i = 0; i < 25; ++i) {
        const int cx = static_cast<int>(origin.x) + offset(rng);
        const int cy = static_cast<int>(origin.y) + offset(rng);
        if (!isWalkableEnemyCell(cx, cy)) continue;
        if (countWallNeighbors(cx, cy) >= 7) continue;
        return sf::Vector2f(static_cast<float>(cx) + 0.5F,
                            static_cast<float>(cy) + 0.5F);
    }
    return origin;
}

// =====================================================================
//  ВОРОГИ
// =====================================================================
void LabyrinthGame::updateEnemies(float dt) {
    for (auto& enemy : enemies) {
        enemy.attackCooldown = std::max(0.0F, enemy.attackCooldown - dt);

        const sf::Vector2f toPlayer = player - enemy.position;
        const float distToPlayer = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
        const bool canSee = distToPlayer < 9.5F &&
                            hasLineOfSight(enemy.position, player, 0.10F);

        const sf::Vector2f toEnemy = enemy.position - player;
        const float playerDistToEnemy = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y);
        float angleDiff = std::atan2(toEnemy.y, toEnemy.x) - playerAngle;
        while (angleDiff >  3.14159F) angleDiff -= 6.28318F;
        while (angleDiff < -3.14159F) angleDiff += 6.28318F;
        enemy.visibleToPlayer = playerDistToEnemy < 13.0F
            && std::abs(angleDiff) < fov * 0.58F
            && hasLineOfSight(player, enemy.position, 0.08F);

        enemy.animPhase += dt * (enemy.state == EnemyChase ? 10.0F : 6.0F);

        if (canSee) {
            enemy.lastSeenPlayerPos = player;
            enemy.hasLastSeen = true;
            if (enemy.state == EnemyWander || enemy.state == EnemySearch) {
                enemy.state      = EnemyAlert;
                enemy.stateTimer = 0.75F;
            }
        }

        switch (enemy.state) {
            case EnemyWander:
                if (enemy.wanderTarget.x == 0.0F && enemy.wanderTarget.y == 0.0F)
                    enemy.wanderTarget = chooseEnemyWanderTarget(enemy.position);
                moveEnemyToward(enemy, enemy.wanderTarget, dt, 0.8F);
                if (std::hypot(enemy.wanderTarget.x - enemy.position.x,
                               enemy.wanderTarget.y - enemy.position.y) < 0.45F)
                    enemy.wanderTarget = chooseEnemyWanderTarget(enemy.position);
                break;

            case EnemyAlert:
                enemy.stateTimer -= dt;
                if (enemy.stateTimer <= 0.0F) {
                    enemy.state  = EnemyChase;
                    enemy.chasing = true;
                }
                break;

            case EnemyChase:
                moveEnemyToward(enemy, player, dt, 1.25F);
                if (!canSee) {
                    enemy.state      = EnemySearch;
                    enemy.stateTimer = 2.8F;
                }
                if (distToPlayer < 1.15F && enemy.attackCooldown <= 0.0F) {
                    enemy.state      = EnemyAttack;
                    enemy.stateTimer = 0.30F;
                    enemy.attackCooldown = 1.2F;
                    hp = std::max(0.0F, hp - 15.0F);
                }
                break;

            case EnemySearch:
                if (enemy.hasLastSeen)
                    moveEnemyToward(enemy, enemy.lastSeenPlayerPos, dt, 1.0F);
                enemy.stateTimer -= dt;
                if (canSee) {
                    enemy.state      = EnemyAlert;
                    enemy.stateTimer = 0.65F;
                } else if (enemy.stateTimer <= 0.0F ||
                           std::hypot(enemy.lastSeenPlayerPos.x - enemy.position.x,
                                      enemy.lastSeenPlayerPos.y - enemy.position.y) < 0.6F) {
                    enemy.state       = EnemyWander;
                    enemy.chasing     = false;
                    enemy.wanderTarget = chooseEnemyWanderTarget(enemy.position);
                }
                break;

            case EnemyAttack:
                enemy.stateTimer -= dt;
                if (enemy.stateTimer <= 0.0F) {
                    enemy.state = canSee ? EnemyChase : EnemySearch;
                    if (enemy.state == EnemySearch) enemy.stateTimer = 2.2F;
                }
                break;

            default:
                enemy.state = EnemyWander;
                break;
        }
    }
}

// =====================================================================
//  КЛЮЧІ / ДВЕРІ / ВИХІД
// =====================================================================
void LabyrinthGame::revealNearbyKeys() {
    const int px = static_cast<int>(player.x);
    const int py = static_cast<int>(player.y);
    for (auto& key : keys) {
        if (key.collected) continue;
        if (std::abs(px - key.position.x) <= 1 &&
            std::abs(py - key.position.y) <= 1)
            key.revealed = true;
    }
}

void LabyrinthGame::collectAtPlayerCell() {
    const int px = static_cast<int>(player.x);
    const int py = static_cast<int>(player.y);
    char& tile = map[py][px];

    if (tile >= '1' && tile <= '3') {
        ++score;          // ← тільки один раз! (було два ++score — це і була помилка)
        tile = '.';
        for (auto& key : keys)
            if (key.position == sf::Vector2i{px, py})
                key.collected = true;
        if (pickupSound.has_value()) pickupSound->play();
    }
}

void LabyrinthGame::unlockDoorAndSpawnExit() {
    if (score != 3 || exitSpawned) return;   // рівно 3 — двері відчиняються
    exitSpawned = true;
    for (auto& row : map) {
        std::replace(row.begin(), row.end(), 'D', '.');
        std::replace(row.begin(), row.end(), 'e', 'E');
    }
}

void LabyrinthGame::checkWin() {
    const int px = static_cast<int>(player.x);
    const int py = static_cast<int>(player.y);
    if (map[py][px] == 'E') gameWon = true;
}

// =====================================================================
//  РЕНДЕР — КОЛІР СТІНИ
// =====================================================================
sf::Color LabyrinthGame::makeSimpleWallColor(float distanceToWall, char hitTile) const {
    if (hitTile == 'D') {
        const int rust = std::max(25, 120 - static_cast<int>(distanceToWall * 8.F));
        return sf::Color(rust, rust / 2, rust / 3);
    }
    const float light = std::clamp(1.0F - (distanceToWall / maxDepth), 0.0F, 1.0F);
    const int tone = static_cast<int>(light * 3.0F);
    return sf::Color(14 + tone * 13, 16 + tone * 13, 20 + tone * 15);
}

// =====================================================================
//  РЕНДЕР — СВІТ (RAYCASTER)
// =====================================================================
void LabyrinthGame::drawFirstPersonWorld() {
    const float sprintZoom = 0.08F * sprintVisualTimer;
    const float jumpZoom   = 0.16F * flightVisualTimer;
    const float zoomRatio  = std::clamp(1.0F - sprintZoom - jumpZoom, 0.74F, 1.0F);
    const float dynamicFov = fov * zoomRatio;
    const float horizonY   = static_cast<float>(screenHeight) / 2.F + cameraBobOffset;
    const float screenShakeX = std::sin(walkWavePhase * 1.25F)
        * (3.0F + sprintVisualTimer * 6.0F + flightVisualTimer * 9.0F);

    std::vector<float> wallDistances(screenWidth, maxDepth);

    // Небо і підлога — темні
    sf::RectangleShape sky(sf::Vector2f(static_cast<float>(screenWidth), horizonY));
    sky.setFillColor(sf::Color(18, 18, 22));
    window.draw(sky);

    sf::RectangleShape ground(sf::Vector2f(static_cast<float>(screenWidth),
                                           static_cast<float>(screenHeight) - horizonY));
    ground.setPosition(sf::Vector2f(0.F, horizonY));
    ground.setFillColor(sf::Color(28, 28, 32));
    window.draw(ground);

    // Стіни
    sf::RectangleShape strip;
    for (unsigned int x = 0; x < screenWidth; ++x) {
        const float rayAngle = (playerAngle - dynamicFov / 2.F)
            + (static_cast<float>(x) / static_cast<float>(screenWidth)) * dynamicFov;
        const sf::Vector2f rayDir{std::cos(rayAngle), std::sin(rayAngle)};

        float distanceToWall = 0.F;
        char  hitTile        = '.';

        while (distanceToWall < maxDepth) {
            distanceToWall += 0.03F;
            const int tx = static_cast<int>(player.x + rayDir.x * distanceToWall);
            const int ty = static_cast<int>(player.y + rayDir.y * distanceToWall);
            if (!isInsideMap(tx, ty)) { distanceToWall = maxDepth; break; }
            const char tile = tileAt(tx, ty);
            if (isBlockingTile(tile)) { hitTile = tile; break; }
        }

        const float corrDist = std::max(0.001F,
            distanceToWall * std::cos(rayAngle - playerAngle));
        wallDistances[x] = corrDist;
        const int wallH   = static_cast<int>(static_cast<float>(screenHeight) / corrDist);
        const int ceiling = std::max(0, static_cast<int>(horizonY) - wallH / 2);
        const int floor   = std::min(static_cast<int>(screenHeight), ceiling + wallH);

        strip.setPosition(sf::Vector2f(static_cast<float>(x) + screenShakeX,
                                       static_cast<float>(ceiling)));
        strip.setSize(sf::Vector2f(1.F,
                                   static_cast<float>(std::max(0, floor - ceiling))));
        strip.setFillColor(makeSimpleWallColor(distanceToWall, hitTile));
        window.draw(strip);
    }

    // NPC
    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) continue;

        const sf::Vector2f toEnemy = enemy.position - player;
        const float dist = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y);
        if (dist < 0.1F || dist >= maxDepth) continue;

        float angleToEnemy = std::atan2(toEnemy.y, toEnemy.x) - playerAngle;
        while (angleToEnemy >  3.14159F) angleToEnemy -= 6.28318F;
        while (angleToEnemy < -3.14159F) angleToEnemy += 6.28318F;
        if (std::abs(angleToEnemy) > dynamicFov * 0.60F) continue;

        const float screenX = ((angleToEnemy + dynamicFov / 2.0F) / dynamicFov)
                              * static_cast<float>(screenWidth);
        const int columnX = static_cast<int>(screenX);
        if (columnX < 0 || columnX >= static_cast<int>(screenWidth)) continue;
        if (dist > wallDistances[columnX]) continue;

        const float attackScale = (enemy.state == EnemyAttack) ? 1.25F : 1.0F;
        const float bodyHeight  = (static_cast<float>(screenHeight) / dist) * 0.95F * attackScale;
        const float bodyWidth   = std::max(8.0F, bodyHeight * 0.30F);
        const float walkSwing   = std::sin(enemy.animPhase) *
            (enemy.state == EnemyWander || enemy.state == EnemyChase ? 6.5F : 2.0F);
        const float headTilt    = (enemy.state == EnemyAlert)
                                  ? std::sin(enemy.animPhase * 0.8F) * 9.0F : 0.0F;
        const float baseY       = horizonY + cameraBobOffset * 0.12F;

        if (enemySpriteAssetsLoaded) {
            const std::vector<sf::Texture>* frameSet = &enemyWalkFrames;
            float animSpeed = 8.0F;
            if (enemy.state == EnemyAttack)      { frameSet = &enemyAttackFrames; animSpeed = 12.0F; }
            else if (enemy.state == EnemyAlert)  { frameSet = &enemyAlertFrames;  animSpeed =  5.0F; }

            if (!frameSet->empty()) {
                const int fi = static_cast<int>(std::abs(enemy.animPhase * animSpeed))
                               % static_cast<int>(frameSet->size());
                sf::Sprite sp((*frameSet)[fi]);
                const sf::Vector2u ts = (*frameSet)[fi].getSize();
                if (ts.x > 0 && ts.y > 0) {
                    sp.setOrigin(sf::Vector2f(ts.x / 2.F, ts.y / 2.F));
                    sp.setPosition(sf::Vector2f(screenX + screenShakeX + headTilt * 0.35F,
                                               baseY + walkSwing));
                    sp.setColor(sf::Color(255, 255, 255, 235));
                    sp.setScale(sf::Vector2f(bodyHeight / ts.y, bodyHeight / ts.y));
                    window.draw(sp);
                    continue;
                }
            }
        }

        // Fallback силует
        sf::RectangleShape body(sf::Vector2f(bodyWidth, bodyHeight * 0.63F));
        body.setOrigin(body.getGeometricCenter());
        body.setPosition(sf::Vector2f(screenX + screenShakeX, baseY + walkSwing));
        body.setFillColor(sf::Color(7, 7, 7));
        window.draw(body);

        sf::CircleShape head(bodyWidth * 0.34F);
        head.setOrigin(head.getGeometricCenter());
        head.setPosition(sf::Vector2f(screenX + screenShakeX + headTilt,
                                      baseY - bodyHeight * 0.40F + walkSwing));
        head.setFillColor(sf::Color::Black);
        window.draw(head);

        sf::RectangleShape armL(sf::Vector2f(bodyWidth * 0.18F, bodyHeight * 0.34F));
        armL.setOrigin(armL.getGeometricCenter());
        armL.setPosition(sf::Vector2f(screenX - bodyWidth * 0.40F + screenShakeX,
                                      baseY - bodyHeight * 0.04F + walkSwing));
        armL.setRotation(sf::degrees(-25.0F + std::sin(enemy.animPhase) * 35.0F));
        armL.setFillColor(sf::Color(10, 10, 10));
        window.draw(armL);

        sf::RectangleShape armR(sf::Vector2f(bodyWidth * 0.18F, bodyHeight * 0.34F));
        armR.setOrigin(armR.getGeometricCenter());
        armR.setPosition(sf::Vector2f(screenX + bodyWidth * 0.40F + screenShakeX,
                                      baseY - bodyHeight * 0.04F + walkSwing));
        armR.setRotation(sf::degrees(25.0F - std::sin(enemy.animPhase) * 35.0F));
        armR.setFillColor(sf::Color(10, 10, 10));
        window.draw(armR);
    }
}

// =====================================================================
//  РУКИ ГРАВЦЯ
// =====================================================================
void LabyrinthGame::drawPlayerHands(bool isWalking, bool isSprinting) {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);

    const float handW = sw * 0.13F;
    const float handH = sh * 0.42F;

    const float baseLeftX  = sw * 0.08F + handBobX;
    const float baseRightX = sw * 0.92F - handBobX;
    const float baseY      = sh * 0.72F + handBobY;

    const float sprintOffsetY = sprintVisualTimer * 28.0F;
    const float sprintOffsetX = sprintVisualTimer * 14.0F;

    const sf::Color skinColor  (38, 28, 22);
    const sf::Color sleeveColor(22, 18, 14);

    const float swingAngle = isWalking ? 8.0F : 1.5F;

    // --- ЛІВА РУКА ---
    sf::RectangleShape leftArm(sf::Vector2f(handW * 0.55F, handH * 0.72F));
    leftArm.setFillColor(sleeveColor);
    leftArm.setOrigin(sf::Vector2f(leftArm.getSize().x * 0.5F, 0.F));
    leftArm.setPosition(sf::Vector2f(baseLeftX - sprintOffsetX, baseY + sprintOffsetY));
    leftArm.setRotation(sf::degrees(-12.0F + std::sin(handSwayPhase) * swingAngle));
    window.draw(leftArm);

    sf::RectangleShape leftHand(sf::Vector2f(handW * 0.52F, handH * 0.22F));
    leftHand.setFillColor(skinColor);
    leftHand.setOrigin(sf::Vector2f(leftHand.getSize().x * 0.5F, 0.F));
    const float leftRad = sf::degrees(-12.0F + std::sin(handSwayPhase) * swingAngle).asRadians();
    leftHand.setPosition(sf::Vector2f(
        baseLeftX - sprintOffsetX + std::sin(leftRad) * leftArm.getSize().y * 0.5F,
        baseY + sprintOffsetY    + std::cos(leftRad) * leftArm.getSize().y));
    leftHand.setRotation(sf::degrees(-12.0F + std::sin(handSwayPhase) * swingAngle));
    window.draw(leftHand);

    // --- ПРАВА РУКА ---
    sf::RectangleShape rightArm(sf::Vector2f(handW * 0.55F, handH * 0.72F));
    rightArm.setFillColor(sleeveColor);
    rightArm.setOrigin(sf::Vector2f(rightArm.getSize().x * 0.5F, 0.F));
    rightArm.setPosition(sf::Vector2f(baseRightX + sprintOffsetX, baseY + sprintOffsetY));
    rightArm.setRotation(sf::degrees(12.0F - std::sin(handSwayPhase) * swingAngle));
    window.draw(rightArm);

    sf::RectangleShape rightHand(sf::Vector2f(handW * 0.52F, handH * 0.22F));
    rightHand.setFillColor(skinColor);
    rightHand.setOrigin(sf::Vector2f(rightHand.getSize().x * 0.5F, 0.F));
    const float rightRad = sf::degrees(12.0F - std::sin(handSwayPhase) * swingAngle).asRadians();
    rightHand.setPosition(sf::Vector2f(
        baseRightX + sprintOffsetX + std::sin(rightRad) * rightArm.getSize().y * 0.5F,
        baseY + sprintOffsetY      + std::cos(rightRad) * rightArm.getSize().y));
    rightHand.setRotation(sf::degrees(12.0F - std::sin(handSwayPhase) * swingAngle));
    window.draw(rightHand);

    (void)isSprinting; // використовується через sprintVisualTimer
}

// =====================================================================
//  ВІНЬЄТКА + ТЕМРЯВА
// =====================================================================
void LabyrinthGame::drawVignette() {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);

    // ===== Плавна радіальна віньєтка через VertexArray =====
    // Центр — прозорий, краї — чорні. Малюємо як набір трикутників від центру.
    const sf::Vector2f center(sw / 2.F, sh / 2.F);
    constexpr int segments = 64; // чим більше — тим плавніше коло
    const float rx = sw * 0.72F; // радіус по X (більше sw/2 — торкається країв)
    const float ry = sh * 0.72F; // радіус по Y

    const sf::Color transparent(0, 0, 0, 0);
    const sf::Color edgeColor(0, 0, 0, 210); // темрява по краях

    sf::VertexArray vignette(sf::PrimitiveType::TriangleFan, segments + 2);
    vignette[0].position = center;
    vignette[0].color    = transparent;

    for (int i = 0; i <= segments; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(segments)
                            * 2.0F * 3.14159265F;
        vignette[i + 1].position = sf::Vector2f(
            center.x + std::cos(angle) * rx,
            center.y + std::sin(angle) * ry
        );
        vignette[i + 1].color = edgeColor;
    }
    window.draw(vignette);

    // ===== Загальна темрява при низькому HP =====
    if (ambientDarknessAlpha > 1.0F) {
        sf::RectangleShape dim(sf::Vector2f(sw, sh));
        dim.setFillColor(sf::Color(0, 0, 0,
            static_cast<uint8_t>(ambientDarknessAlpha)));
        window.draw(dim);
    }

    // ===== Пульсуюча червона рамка при hp < 40 =====
    if (hp < 40.0F) {
        const float pulse    = std::abs(std::sin(flickerPhase * 2.5F));
        const float ratio    = (40.0F - hp) / 40.0F;
        const uint8_t redAlpha = static_cast<uint8_t>(
            std::clamp(ratio * 180.0F * pulse, 0.0F, 200.0F));

        // Плавна червона віньєтка — теж через трикутники
        const float rrx = sw * 0.55F;
        const float rry = sh * 0.55F;
        const sf::Color redEdge(180, 0, 0, redAlpha);

        sf::VertexArray redVig(sf::PrimitiveType::TriangleFan, segments + 2);
        redVig[0].position = center;
        redVig[0].color    = sf::Color(0, 0, 0, 0);
        for (int i = 0; i <= segments; ++i) {
            const float angle = static_cast<float>(i) / static_cast<float>(segments)
                                * 2.0F * 3.14159265F;
            redVig[i + 1].position = sf::Vector2f(
                center.x + std::cos(angle) * rrx,
                center.y + std::sin(angle) * rry
            );
            redVig[i + 1].color = redEdge;
        }
        window.draw(redVig);
    }
}

// =====================================================================
//  МІНІКАРТА
// =====================================================================
void LabyrinthGame::drawMiniMap() {
    constexpr float miniTile      = 12.F;
    constexpr int   visionR       = 6;
    const sf::Vector2f center(118.F, 118.F);
    const float circleR = miniTile * static_cast<float>(visionR) + 4.F;

    sf::CircleShape bg(circleR);
    bg.setOrigin(bg.getGeometricCenter());
    bg.setPosition(center);
    bg.setFillColor(sf::Color(10, 10, 15, 210));
    bg.setOutlineThickness(2.F);
    bg.setOutlineColor(sf::Color(150, 150, 160));
    window.draw(bg);

    sf::RectangleShape tile(sf::Vector2f(miniTile - 1.F, miniTile - 1.F));
    const int px = static_cast<int>(player.x);
    const int py = static_cast<int>(player.y);

    for (int dy = -visionR; dy <= visionR; ++dy)
        for (int dx = -visionR; dx <= visionR; ++dx) {
            if (std::sqrt(static_cast<float>(dx * dx + dy * dy)) >
                static_cast<float>(visionR)) continue;
            const int mx = px + dx, my = py + dy;
            if (!isInsideMap(mx, my)) continue;

            char t = map[my][mx];
            if (t >= '1' && t <= '3') {
                bool rev = false;
                for (const auto& key : keys)
                    if (key.position == sf::Vector2i{mx, my}) { rev = key.revealed; break; }
                if (!rev) t = '.';
            }

            if      (t == '#') tile.setFillColor(sf::Color(42, 42, 48));
            else if (t == 'D') tile.setFillColor(sf::Color(120, 70, 30));
            else if (t >= '1' && t <= '3') tile.setFillColor(sf::Color::Yellow);
            else if (t == 'E') tile.setFillColor(sf::Color(155, 70, 220));
            else               tile.setFillColor(sf::Color(180, 180, 180));

            tile.setPosition(sf::Vector2f(center.x + dx * miniTile,
                                          center.y + dy * miniTile));
            window.draw(tile);
        }

    sf::CircleShape p(4.F);
    p.setFillColor(sf::Color::Cyan);
    p.setOrigin(p.getGeometricCenter());
    p.setPosition(center);
    window.draw(p);

    sf::Vertex line[2];
    line[0].position = center; line[0].color = sf::Color::Cyan;
    line[1].position = sf::Vector2f(center.x + std::cos(playerAngle) * 20.F,
                                    center.y + std::sin(playerAngle) * 20.F);
    line[1].color = sf::Color::Cyan;
    window.draw(line, 2, sf::PrimitiveType::Lines);

    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) continue;
        const int dx = static_cast<int>(enemy.position.x) - px;
        const int dy = static_cast<int>(enemy.position.y) - py;
        if (std::sqrt(static_cast<float>(dx * dx + dy * dy)) >
            static_cast<float>(visionR)) continue;

        sf::CircleShape e(3.8F);
        e.setOrigin(e.getGeometricCenter());
        e.setPosition(sf::Vector2f(center.x + static_cast<float>(dx) * miniTile,
                                   center.y + static_cast<float>(dy) * miniTile));
        e.setFillColor(sf::Color::Black);
        e.setOutlineThickness(1.F);
        e.setOutlineColor(sf::Color(120, 120, 120));
        window.draw(e);
    }
}

// =====================================================================
//  ПОВНА КАРТА
// =====================================================================
void LabyrinthGame::drawFullMapOverlay() {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(screenWidth),
                                        static_cast<float>(screenHeight)));
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);

    const float maxW     = screenWidth  * 0.78F;
    const float maxH     = screenHeight * 0.84F;
    const float tileSize = std::min(maxW / static_cast<float>(mapWidth),
                                    maxH / static_cast<float>(mapHeight));
    const float startX   = (static_cast<float>(screenWidth)  - tileSize * mapWidth)  / 2.F;
    const float startY   = (static_cast<float>(screenHeight) - tileSize * mapHeight) / 2.F;

    sf::RectangleShape tile(sf::Vector2f(tileSize - 0.2F, tileSize - 0.2F));
    for (int y = 0; y < mapHeight; ++y)
        for (int x = 0; x < mapWidth; ++x) {
            const char t = map[y][x];
            if      (t == '#') tile.setFillColor(sf::Color(34, 34, 40));
            else if (t == 'D') tile.setFillColor(sf::Color(120, 70, 30));
            else if (t == 'E') tile.setFillColor(sf::Color(155, 70, 220));
            else               tile.setFillColor(sf::Color(165, 165, 170));
            tile.setPosition(sf::Vector2f(startX + x * tileSize, startY + y * tileSize));
            window.draw(tile);
        }

    sf::CircleShape pd(std::max(2.0F, tileSize * 0.35F));
    pd.setFillColor(sf::Color::Cyan);
    pd.setOrigin(pd.getGeometricCenter());
    pd.setPosition(sf::Vector2f(startX + player.x * tileSize,
                                startY + player.y * tileSize));
    window.draw(pd);

    sf::Vertex fl[2];
    fl[0].position = sf::Vector2f(startX + player.x * tileSize,
                                  startY + player.y * tileSize);
    fl[0].color = sf::Color::Cyan;
    fl[1].position = sf::Vector2f(
        startX + (player.x + std::cos(playerAngle) * 2.0F) * tileSize,
        startY + (player.y + std::sin(playerAngle) * 2.0F) * tileSize);
    fl[1].color = sf::Color::Cyan;
    window.draw(fl, 2, sf::PrimitiveType::Lines);

    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) continue;
        sf::CircleShape m(std::max(2.2F, tileSize * 0.30F));
        m.setOrigin(m.getGeometricCenter());
        m.setPosition(sf::Vector2f(startX + enemy.position.x * tileSize,
                                   startY + enemy.position.y * tileSize));
        m.setFillColor(sf::Color::Black);
        m.setOutlineThickness(1.F);
        m.setOutlineColor(sf::Color(110, 110, 120));
        window.draw(m);
    }

    if (fontLoaded) {
        sf::Text title(font, "ПОВНА КАРТА (M - закрити)", 24);
        title.setFillColor(sf::Color::White);
        title.setPosition(sf::Vector2f(startX, startY - 34.F));
        window.draw(title);
    }
}

// =====================================================================
//  HUD
// =====================================================================
void LabyrinthGame::drawHud() {
    constexpr float panelW = 220.F;
    constexpr float panelH = 74.F;
    const float panelX = static_cast<float>(screenWidth)  - panelW - 16.F;
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
        orb.setPosition(sf::Vector2f(panelX + 30.F + i * 30.F,
                                     panelY + panelH / 2.F + 10.F));
        orb.setFillColor(i < score ? sf::Color(235, 205, 70) : sf::Color(55, 55, 60));
        orb.setOutlineThickness(1.F);
        orb.setOutlineColor(sf::Color(130, 130, 140));
        window.draw(orb);
    }

    if (fontLoaded) {
        sf::Text corner(font);
        corner.setCharacterSize(18);
        corner.setFillColor(sf::Color(220, 220, 230));
        corner.setPosition(sf::Vector2f(panelX + 16.F, panelY + 10.F));
        corner.setString("КЛЮЧІ: " + std::to_string(score) + "/3");
        window.draw(corner);

        sf::Text controls(font);
        controls.setCharacterSize(18);
        controls.setFillColor(sf::Color::White);
        controls.setPosition(sf::Vector2f(10.F,
                             static_cast<float>(screenHeight - 52)));
        controls.setString(
            "W/S - рух, A/D - поворот, Shift - біг, Shift+Space - стрибок-політ, M - карта, ESC - вихід");
        window.draw(controls);
    }

    const float colX    = 22.F;
    const float colBot  = static_cast<float>(screenHeight) - 24.F;
    const float barW    = 20.F;
    const float barH    = 160.F;
    const float gap     = 14.F;

    const auto drawBar = [&](float x, float ratio, const sf::Color& fill) {
        sf::RectangleShape frame(sf::Vector2f(barW, barH));
        frame.setPosition(sf::Vector2f(x, colBot - barH));
        frame.setFillColor(sf::Color(16, 16, 20, 220));
        frame.setOutlineThickness(2.F);
        frame.setOutlineColor(sf::Color(210, 210, 220));
        window.draw(frame);

        const float valueH = (barH - 4.F) * std::clamp(ratio, 0.0F, 1.0F);
        sf::RectangleShape fillRect(sf::Vector2f(barW - 4.F, valueH));
        fillRect.setPosition(sf::Vector2f(x + 2.F, colBot - 2.F - valueH));
        fillRect.setFillColor(fill);
        window.draw(fillRect);
    };

    drawBar(colX,          hp      / 100.0F, sf::Color(220, 40, 40));
    drawBar(colX + barW + gap, stamina / 100.0F, sf::Color(45, 120, 255));

    if (fontLoaded) {
        sf::Text hpMark(font, "+", 34);
        hpMark.setFillColor(sf::Color(245, 50, 50));
        hpMark.setPosition(sf::Vector2f(colX + 2.F, colBot + 4.F));
        window.draw(hpMark);

        sf::Text stMark(font, "S", 22);
        stMark.setFillColor(sf::Color(90, 150, 255));
        stMark.setPosition(sf::Vector2f(colX + barW + gap + 3.F, colBot + 10.F));
        window.draw(stMark);
    }
}

// =====================================================================
//  GAME OVER
// =====================================================================
void LabyrinthGame::drawGameOver() {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);

    sf::RectangleShape bg(sf::Vector2f(sw, sh));
    bg.setFillColor(sf::Color(0, 0, 0));
    window.draw(bg);

    const float pulse = std::abs(std::sin(portalClock.getElapsedTime().asSeconds() * 1.2F));
    sf::CircleShape glow(280.F);
    glow.setOrigin(glow.getGeometricCenter());
    glow.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F));
    glow.setFillColor(sf::Color(80, 0, 0,
        static_cast<uint8_t>(60.F + pulse * 60.F)));
    window.draw(glow);

    if (!fontLoaded) return;

    sf::Text title(font, "GAME OVER", 96);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(200, 20, 20));
    const auto tb = title.getLocalBounds();
    title.setOrigin(sf::Vector2f(tb.size.x / 2.F, tb.size.y / 2.F));
    title.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F - 60.F));
    window.draw(title);

    sf::Text sub(font, "Ти помер у темряві...", 32);
    sub.setFillColor(sf::Color(160, 160, 160));
    const auto sb = sub.getLocalBounds();
    sub.setOrigin(sf::Vector2f(sb.size.x / 2.F, sb.size.y / 2.F));
    sub.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F + 30.F));
    window.draw(sub);

    sf::Text tip(font, "ESC — вийти", 24);
    tip.setFillColor(sf::Color(100, 100, 110));
    const auto tipb = tip.getLocalBounds();
    tip.setOrigin(sf::Vector2f(tipb.size.x / 2.F, tipb.size.y / 2.F));
    tip.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F + 90.F));
    window.draw(tip);
}

// =====================================================================
//  ПОРТАЛ (ПЕРЕМОГА)
// =====================================================================
void LabyrinthGame::drawPortalScreen() {
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(screenWidth),
                                       static_cast<float>(screenHeight)));
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

    if (!fontLoaded) return;

    sf::Text title(font, "СИСТЕМА ЗЛАМАННЯ. ВИ ВІЛЬНІ", 42);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color::White);
    title.setPosition(sf::Vector2f(120.F,
                      static_cast<float>(screenHeight) - 170.F));
    window.draw(title);

    sf::Text tip(font, "Натисни ESC, щоб закрити гру", 26);
    tip.setFillColor(sf::Color(220, 220, 230));
    tip.setPosition(sf::Vector2f(300.F,
                    static_cast<float>(screenHeight) - 115.F));
    window.draw(tip);
}

// =====================================================================
//  РЕНДЕР
// =====================================================================
void LabyrinthGame::render() {
    window.clear(sf::Color(0, 0, 0));

    if (gameover) {
        drawGameOver();
    } else if (gameWon) {
        drawPortalScreen();
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
        drawScreamer();
    }

    window.display();
}