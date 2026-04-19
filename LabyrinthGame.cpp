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
    EnemyAlert = 1,
    EnemyChase = 2,
    EnemySearch = 3,
    EnemyAttack = 4
};
}

LabyrinthGame::LabyrinthGame()
    : window(sf::VideoMode({screenWidth, screenHeight}), "Лабіринт Спотвореної Реальності - First Person (SFML)") {
    window.setFramerateLimit(60);

    // ===== [Налаштування карти гри] =====
    buildLargeMap();
    placeKeysRandomly();
    spawnEnemiesRandomly();

    // ===== [Налаштування шрифту HUD] =====
    fontLoaded = font.openFromFile("arial.ttf");
    if (!fontLoaded) {
        std::cerr << "[Попередження] Не знайдено arial.ttf. Текст HUD буде вимкнено.\n";
    }

    loadEnemySpriteAssets();
}

void LabyrinthGame::run() {
    while (window.isOpen()) {
        const float dt = deltaClock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

bool LabyrinthGame::isInsideMap(int x, int y) const {
    return x >= 0 && x < mapWidth && y >= 0 && y < mapHeight;
}

char LabyrinthGame::tileAt(int x, int y) const {
    return map[y][x];
}

bool LabyrinthGame::isBlockingTile(char tile) const {
    if (tile == '#') {
        return true;
    }
    if (tile == 'D' && score < 3) {
        return true;
    }
    return false;
}

int LabyrinthGame::countWallNeighbors(int x, int y) const {
    int walls = 0;
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) {
                continue;
            }
            const int nx = x + ox;
            const int ny = y + oy;
            if (!isInsideMap(nx, ny) || map[ny][nx] == '#') {
                ++walls;
            }
        }
    }
    return walls;
}

// ===== [Генерація карти 32x32 зі "старим" стилем стін (коридори/кімнати)] =====
void LabyrinthGame::buildLargeMap() {
    const std::vector<std::vector<std::string>> chunks = {
        {
            "################",
            "#......#.......#",
            "#.####.#.#####.#",
            "#.#..#.#.....#.#",
            "#.#..#.###.#.#.#",
            "#.#..#...#.#...#",
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
            "#...#.####.#...#",
            "###.#.#..#.#.###",
            "#...#.#..#.#...#",
            "#.###.####.###.#",
            "#.#..........#.#",
            "#.#.########.#.#",
            "#.#..........#.#",
            "#.############.#",
            "#..............#",
            "################",
        },
        {
            "################",
            "#..#........#..#",
            "#..#.######.#..#",
            "#..#.#....#.#..#",
            "#....#....#....#",
            "####.#.##.#.####",
            "#....#....#....#",
            "#.##.######.##.#",
            "#..............#",
            "#.##.######.##.#",
            "#....#....#....#",
            "####.#.##.#.####",
            "#....#....#....#",
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
            "#.#.#......#.#.#",
            "#.#.#.####.#.#.#",
            "#...#.#..#.#...#",
            "###.#.#..#.#.###",
            "#...#.#..#.#...#",
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
            int idx = pick(rng);
            int guard = 0;
            while ((idx == prev || idx == prevRow[tx]) && guard < 20) {
                idx = pick(rng);
                ++guard;
            }

            const auto& chunk = chunks[idx];
            for (int y = 0; y < chunkSize; ++y) {
                for (int x = 0; x < chunkSize; ++x) {
                    map[ty * chunkSize + y][tx * chunkSize + x] = chunk[y][x];
                }
            }

            prev = idx;
            prevRow[tx] = idx;
        }
    }

    // Проходи між блоками.
    for (int c = 1; c < chunkGrid; ++c) {
        const int seamXLeft = c * chunkSize - 1;
        const int seamXRight = c * chunkSize;
        for (int y = 3; y < mapHeight - 3; y += 6) {
            for (int k = 0; k < 3 && y + k < mapHeight - 1; ++k) {
                map[y + k][seamXLeft] = '.';
                map[y + k][seamXRight] = '.';
            }
        }

        const int seamYTop = c * chunkSize - 1;
        const int seamYBottom = c * chunkSize;
        for (int x = 3; x < mapWidth - 3; x += 6) {
            for (int k = 0; k < 3 && x + k < mapWidth - 1; ++k) {
                map[seamYTop][x + k] = '.';
                map[seamYBottom][x + k] = '.';
            }
        }
    }

    // Старт і зона виходу.
    player = sf::Vector2f(1.5F, 1.5F);
    map[1][1] = '.';
    map[1][2] = '.';
    map[2][1] = '.';

    map[mapHeight - 4][mapWidth - 4] = 'D';
    map[mapHeight - 4][mapWidth - 3] = 'e';
    map[mapHeight - 5][mapWidth - 4] = '.';
    map[mapHeight - 4][mapWidth - 5] = '.';

    // Гарантований коридор до фінальної зони.
    for (int x = 1; x <= mapWidth - 5; ++x) {
        map[2][x] = '.';
    }
    for (int y = 2; y <= mapHeight - 4; ++y) {
        map[y][mapWidth - 5] = '.';
    }
}

// ===== [Випадкове розміщення 3 ключів при кожному запуску] =====
void LabyrinthGame::placeKeysRandomly() {
    keys.clear();

    std::vector<sf::Vector2i> candidates;
    candidates.reserve(mapWidth * mapHeight);

    for (int y = 1; y < mapHeight - 1; ++y) {
        for (int x = 1; x < mapWidth - 1; ++x) {
            if (map[y][x] != '.') {
                continue;
            }
            if (x <= 4 && y <= 4) {
                continue;
            }
            if (x >= mapWidth - 7 && y >= mapHeight - 7) {
                continue;
            }
            candidates.push_back(sf::Vector2i{x, y});
        }
    }

    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::vector<sf::Vector2i> picked;
    for (const auto& pos : candidates) {
        bool farEnough = true;
        for (const auto& p : picked) {
            const int dx = p.x - pos.x;
            const int dy = p.y - pos.y;
            if (dx * dx + dy * dy < 120) {
                farEnough = false;
                break;
            }
        }
        if (!farEnough) {
            continue;
        }
        picked.push_back(pos);
        if (picked.size() == 3) {
            break;
        }
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

    for (int y = 2; y < mapHeight - 2; ++y) {
        for (int x = 2; x < mapWidth - 2; ++x) {
            if (map[y][x] != '.') {
                continue;
            }

            if (x < 6 && y < 6) {
                continue;
            }

            candidates.emplace_back(static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F);
        }
    }

    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    constexpr int enemyCount = 4;
    for (const auto& candidate : candidates) {
        bool farEnough = true;
        for (const auto& enemy : enemies) {
            const sf::Vector2f d = enemy.position - candidate;
            if (d.x * d.x + d.y * d.y < 70.0F) {
                farEnough = false;
                break;
            }
        }
        if (!farEnough) {
            continue;
        }

        EnemyInfo enemy;
        enemy.position = candidate;
        enemy.speed = 1.3F;
        enemy.state = EnemyWander;
        enemy.wanderTarget = candidate;
        enemies.push_back(enemy);

        if (static_cast<int>(enemies.size()) >= enemyCount) {
            break;
        }
    }
}

bool LabyrinthGame::loadEnemyFrameSet(const std::string& patternPrefix, int count, std::vector<sf::Texture>& outFrames) {
    outFrames.clear();
    outFrames.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        sf::Texture frame;
        const std::string path = patternPrefix + std::to_string(i) + ".png";
        if (!frame.loadFromFile(path)) {
            return false;
        }
        frame.setSmooth(true);
        outFrames.push_back(std::move(frame));
    }

    return !outFrames.empty();
}

void LabyrinthGame::loadEnemySpriteAssets() {
    const bool walkOk = loadEnemyFrameSet("assets/npc/walk_", 6, enemyWalkFrames);
    const bool attackOk = loadEnemyFrameSet("assets/npc/attack_", 4, enemyAttackFrames);
    const bool alertOk = loadEnemyFrameSet("assets/npc/alert_", 3, enemyAlertFrames);

    // Найпростіший fallback: один файл model.png на всі стани.
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

    // Мінімальний набір: або walk-кадри, або один model.png.
    enemySpriteAssetsLoaded = walkOk || singleModelOk;
    if (enemySpriteAssetsLoaded && !attackOk) {
        enemyAttackFrames = enemyWalkFrames; // fallback на walk
    }
    if (enemySpriteAssetsLoaded && !alertOk) {
        enemyAlertFrames = enemyWalkFrames; // fallback на walk
    }

    if (!enemySpriteAssetsLoaded) {
        const bool hasFbxSource = std::filesystem::exists("assets/npc/model.fbx");
        enemyWalkFrames.clear();
        enemyAttackFrames.clear();
        enemyAlertFrames.clear();
        std::cerr << "[Попередження] NPC спрайти не знайдені (мінімум walk_* або model.png). Використовується fallback-силует.\n";
        if (hasFbxSource) {
            std::cerr << "[INFO] Знайдено assets/npc/model.fbx. Згенеруй model.png командою з README (Blender headless).\n";
        }
    } else {
        std::cerr << "[INFO] NPC спрайти завантажено. walk=" << enemyWalkFrames.size()
            << ", attack=" << enemyAttackFrames.size() << ", alert=" << enemyAlertFrames.size() << '\n';
    }
}

void LabyrinthGame::processEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                window.close();
            }
            if (keyPressed->code == sf::Keyboard::Key::M) {
                showFullMap = !showFullMap;
            }
        }
    }
}

void LabyrinthGame::update(float dt) {
    if (gameWon) {
        return;
    }

    // ===== [Налаштування руху гравця] =====
    const float rotationSpeed = 1.8F;
    const float baseMoveSpeed = 3.0F;
    const float sprintMultiplier = 1.45F;
    const float flightMultiplier = 2.1F;
    const float sprintDrainPerSecond = 16.0F;
    const float flightDrainPerSecond = 45.0F;
    const float staminaRegenPerSecond = 20.0F;

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
    const bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    const bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    bool isSprinting = false;
    bool isFlightJump = false;
    float moveSpeed = baseMoveSpeed;

    if (shiftPressed && stamina > 1.0F && isWalking) {
        isSprinting = true;
        moveSpeed *= sprintMultiplier;
        stamina -= sprintDrainPerSecond * dt;
        sprintVisualTimer = std::min(1.0F, sprintVisualTimer + dt * 2.8F);
    } else {
        sprintVisualTimer = std::max(0.0F, sprintVisualTimer - dt * 2.2F);
    }

    if (shiftPressed && spacePressed && stamina > 1.0F) {
        isFlightJump = true;
        moveSpeed *= flightMultiplier;
        stamina -= flightDrainPerSecond * dt;
        flightVisualTimer = std::min(0.35F, flightVisualTimer + dt * 2.2F);
    } else {
        flightVisualTimer = std::max(0.0F, flightVisualTimer - dt * 2.6F);
    }

    if (!isSprinting && !isFlightJump) {
        stamina += staminaRegenPerSecond * dt;
    }

    stamina = std::clamp(stamina, 0.0F, 100.0F);
    movePlayer(moveDir, moveSpeed * dt);

    // ===== [Пружиниста хода / хвиля камери] =====
    if (isWalking) {
        const float walkFreq = 10.5F + sprintVisualTimer * 4.0F + flightVisualTimer * 2.5F;
        const float walkAmp = 6.0F + sprintVisualTimer * 5.0F + flightVisualTimer * 7.0F;
        walkWavePhase += dt * walkFreq;
        cameraBobOffset = std::sin(walkWavePhase) * walkAmp;
    } else {
        // ===== [Легке коливання в спокої: ефект "паморочиться голова"] =====
        idleSwayPhase += dt * 1.9F;
        cameraBobOffset = std::sin(idleSwayPhase) * 1.4F;
    }

    if (flightVisualTimer > 0.0F) {
        cameraBobOffset -= 11.0F * flightVisualTimer;
    }

    revealNearbyKeys();
    collectAtPlayerCell();
    unlockDoorAndSpawnExit();
    updateEnemies(dt);
    checkWin();
}

void LabyrinthGame::movePlayer(const sf::Vector2f& dir, float distanceStep) {
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

bool LabyrinthGame::hasLineOfSight(const sf::Vector2f& from, const sf::Vector2f& to, float step) const {
    const sf::Vector2f delta = to - from;
    const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (distance <= 0.001F) {
        return true;
    }

    const sf::Vector2f dir = delta / distance;
    for (float t = 0.0F; t < distance; t += step) {
        const sf::Vector2f p = from + dir * t;
        const int tx = static_cast<int>(p.x);
        const int ty = static_cast<int>(p.y);
        if (!isInsideMap(tx, ty)) {
            return false;
        }
        if (isBlockingTile(tileAt(tx, ty))) {
            return false;
        }
    }
    return true;
}

bool LabyrinthGame::isWalkableEnemyCell(int x, int y) const {
    if (!isInsideMap(x, y)) {
        return false;
    }
    const char tile = tileAt(x, y);
    return tile == '.' || tile == 'E' || (tile >= '1' && tile <= '3');
}

void LabyrinthGame::moveEnemyToward(EnemyInfo& enemy, const sf::Vector2f& target, float dt, float speedScale) {
    sf::Vector2f delta = target - enemy.position;
    const float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (len <= 0.001F) {
        return;
    }

    const sf::Vector2f dir = delta / len;
    sf::Vector2f candidate = enemy.position + dir * enemy.speed * speedScale * dt;

    const int tx = static_cast<int>(candidate.x);
    const int ty = static_cast<int>(candidate.y);
    if (isWalkableEnemyCell(tx, ty)) {
        enemy.position = candidate;
    }
}

sf::Vector2f LabyrinthGame::chooseEnemyWanderTarget(const sf::Vector2f& origin) const {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> offset(-6, 6);

    for (int i = 0; i < 25; ++i) {
        const int cx = static_cast<int>(origin.x) + offset(rng);
        const int cy = static_cast<int>(origin.y) + offset(rng);
        if (!isWalkableEnemyCell(cx, cy)) {
            continue;
        }
        if (countWallNeighbors(cx, cy) >= 7) {
            continue;
        }
        return sf::Vector2f(static_cast<float>(cx) + 0.5F, static_cast<float>(cy) + 0.5F);
    }

    return origin;
}

void LabyrinthGame::updateEnemies(float dt) {
    const sf::Vector2f toPlayerBase = player;

    for (auto& enemy : enemies) {
        enemy.attackCooldown = std::max(0.0F, enemy.attackCooldown - dt);

        const sf::Vector2f toPlayer = toPlayerBase - enemy.position;
        const float distanceToPlayer = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
        const bool enemyCanSeePlayer = distanceToPlayer < 9.5F && hasLineOfSight(enemy.position, player, 0.10F);

        const sf::Vector2f toEnemy = enemy.position - player;
        const float playerDistToEnemy = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y);
        const float playerAngleToEnemy = std::atan2(toEnemy.y, toEnemy.x);
        float playerAngleDiff = playerAngleToEnemy - playerAngle;
        while (playerAngleDiff > 3.14159F) {
            playerAngleDiff -= 6.28318F;
        }
        while (playerAngleDiff < -3.14159F) {
            playerAngleDiff += 6.28318F;
        }
        enemy.visibleToPlayer = playerDistToEnemy < 13.0F
            && std::abs(playerAngleDiff) < fov * 0.58F
            && hasLineOfSight(player, enemy.position, 0.08F);

        enemy.animPhase += dt * (enemy.state == EnemyChase ? 10.0F : 6.0F);

        if (enemyCanSeePlayer) {
            enemy.lastSeenPlayerPos = player;
            enemy.hasLastSeen = true;
            if (enemy.state == EnemyWander || enemy.state == EnemySearch) {
                enemy.state = EnemyAlert;
                enemy.stateTimer = 0.75F; // спец-анімація "нахил голови"
            }
        }

        switch (enemy.state) {
            case EnemyWander: {
                if (enemy.wanderTarget.x == 0.0F && enemy.wanderTarget.y == 0.0F) {
                    enemy.wanderTarget = chooseEnemyWanderTarget(enemy.position);
                }
                moveEnemyToward(enemy, enemy.wanderTarget, dt, 0.8F);
                if (std::hypot(enemy.wanderTarget.x - enemy.position.x, enemy.wanderTarget.y - enemy.position.y) < 0.45F) {
                    enemy.wanderTarget = chooseEnemyWanderTarget(enemy.position);
                }
                break;
            }
            case EnemyAlert: {
                enemy.stateTimer -= dt;
                if (enemy.stateTimer <= 0.0F) {
                    enemy.state = EnemyChase;
                    enemy.chasing = true;
                }
                break;
            }
            case EnemyChase: {
                moveEnemyToward(enemy, player, dt, 1.25F);
                if (!enemyCanSeePlayer) {
                    enemy.state = EnemySearch;
                    enemy.stateTimer = 2.8F;
                }
                if (distanceToPlayer < 1.15F && enemy.attackCooldown <= 0.0F) {
                    enemy.state = EnemyAttack;
                    enemy.stateTimer = 0.30F;
                    enemy.attackCooldown = 1.2F;
                    hp = std::max(0.0F, hp - 15.0F);
                }
                break;
            }
            case EnemySearch: {
                if (enemy.hasLastSeen) {
                    moveEnemyToward(enemy, enemy.lastSeenPlayerPos, dt, 1.0F);
                }
                enemy.stateTimer -= dt;
                if (enemyCanSeePlayer) {
                    enemy.state = EnemyAlert;
                    enemy.stateTimer = 0.65F;
                } else if (enemy.stateTimer <= 0.0F
                    || std::hypot(enemy.lastSeenPlayerPos.x - enemy.position.x, enemy.lastSeenPlayerPos.y - enemy.position.y) < 0.6F) {
                    enemy.state = EnemyWander;
                    enemy.chasing = false;
                    enemy.wanderTarget = chooseEnemyWanderTarget(enemy.position);
                }
                break;
            }
            case EnemyAttack: {
                enemy.stateTimer -= dt;
                if (enemy.stateTimer <= 0.0F) {
                    enemy.state = enemyCanSeePlayer ? EnemyChase : EnemySearch;
                    if (enemy.state == EnemySearch) {
                        enemy.stateTimer = 2.2F;
                    }
                }
                break;
            }
            default:
                enemy.state = EnemyWander;
                break;
        }

    }
}

void LabyrinthGame::revealNearbyKeys() {
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

void LabyrinthGame::collectAtPlayerCell() {
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

void LabyrinthGame::unlockDoorAndSpawnExit() {
    if (score != 3 || exitSpawned) {
        return;
    }

    exitSpawned = true;
    for (auto& row : map) {
        std::replace(row.begin(), row.end(), 'D', '.');
        std::replace(row.begin(), row.end(), 'e', 'E');
    }
}

void LabyrinthGame::checkWin() {
    const int px = static_cast<int>(player.x);
    const int py = static_cast<int>(player.y);
    if (map[py][px] == 'E') {
        gameWon = true;
    }
}

sf::Color LabyrinthGame::makeSimpleWallColor(float distanceToWall, char hitTile) const {
    if (hitTile == 'D') {
        const int rust = std::max(35, 145 - static_cast<int>(distanceToWall * 8.F));
        return sf::Color(rust, rust / 2, rust / 3);
    }

    const float light = std::clamp(1.0F - (distanceToWall / maxDepth), 0.0F, 1.0F);
    const int tone = static_cast<int>(light * 4.0F);

    const int r = 24 + tone * 18;
    const int g = 27 + tone * 18;
    const int b = 33 + tone * 20;
    return sf::Color(r, g, b);
}

void LabyrinthGame::drawFirstPersonWorld() {
    sf::RectangleShape strip;
    const float sprintZoom = 0.08F * sprintVisualTimer;
    const float jumpZoom = 0.16F * flightVisualTimer;
    const float zoomRatio = std::clamp(1.0F - sprintZoom - jumpZoom, 0.74F, 1.0F);
    const float dynamicFov = fov * zoomRatio;
    const float horizonY = static_cast<float>(screenHeight) / 2.F + cameraBobOffset;
    const float screenShakeX = std::sin(walkWavePhase * 1.25F) * (2.0F + sprintVisualTimer * 4.0F + flightVisualTimer * 6.0F);
    std::vector<float> wallDistances(screenWidth, maxDepth);

    sf::RectangleShape sky(sf::Vector2f(static_cast<float>(screenWidth), horizonY));
    sky.setFillColor(sf::Color(206, 206, 210));
    window.draw(sky);

    sf::RectangleShape ground(sf::Vector2f(static_cast<float>(screenWidth), static_cast<float>(screenHeight) - horizonY));
    ground.setPosition(sf::Vector2f(0.F, horizonY));
    ground.setFillColor(sf::Color(53, 53, 58));
    window.draw(ground);

    for (unsigned int x = 0; x < screenWidth; ++x) {
        const float rayAngle =
            (playerAngle - dynamicFov / 2.F) + (static_cast<float>(x) / static_cast<float>(screenWidth)) * dynamicFov;
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
        wallDistances[x] = correctedDistance;
        const int wallHeight = static_cast<int>(static_cast<float>(screenHeight) / correctedDistance);
        const int ceiling = std::max(0, static_cast<int>(horizonY) - wallHeight / 2);
        const int floor = std::min(static_cast<int>(screenHeight), ceiling + wallHeight);

        strip.setPosition(sf::Vector2f(static_cast<float>(x) + screenShakeX, static_cast<float>(ceiling)));
        strip.setSize(sf::Vector2f(1.F, static_cast<float>(std::max(0, floor - ceiling))));
        strip.setFillColor(makeSimpleWallColor(distanceToWall, hitTile));
        window.draw(strip);
    }

    // ===== [Малюємо NPC як чорні "модельки" людей поверх стін] =====
    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) {
            continue;
        }

        const sf::Vector2f toEnemy = enemy.position - player;
        const float dist = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y);
        if (dist < 0.1F || dist >= maxDepth) {
            continue;
        }

        float angleToEnemy = std::atan2(toEnemy.y, toEnemy.x) - playerAngle;
        while (angleToEnemy > 3.14159F) {
            angleToEnemy -= 6.28318F;
        }
        while (angleToEnemy < -3.14159F) {
            angleToEnemy += 6.28318F;
        }
        if (std::abs(angleToEnemy) > dynamicFov * 0.60F) {
            continue;
        }

        const float screenX = ((angleToEnemy + dynamicFov / 2.0F) / dynamicFov) * static_cast<float>(screenWidth);
        const int columnX = static_cast<int>(screenX);
        if (columnX < 0 || columnX >= static_cast<int>(screenWidth)) {
            continue;
        }
        if (dist > wallDistances[columnX]) {
            continue; // за стіною
        }

        const float attackScale = (enemy.state == EnemyAttack) ? 1.25F : 1.0F;
        const float bodyHeight = (static_cast<float>(screenHeight) / dist) * 0.95F * attackScale;
        const float bodyWidth = std::max(8.0F, bodyHeight * 0.30F);
        const float walkSwing = std::sin(enemy.animPhase) * (enemy.state == EnemyWander || enemy.state == EnemyChase ? 6.5F : 2.0F);
        const float headTilt = (enemy.state == EnemyAlert) ? (std::sin(enemy.animPhase * 0.8F) * 9.0F) : 0.0F;
        const float baseY = horizonY + cameraBobOffset * 0.12F;

        if (enemySpriteAssetsLoaded) {
            const std::vector<sf::Texture>* frameSet = &enemyWalkFrames;
            float animSpeed = 8.0F;
            if (enemy.state == EnemyAttack) {
                frameSet = &enemyAttackFrames;
                animSpeed = 12.0F;
            } else if (enemy.state == EnemyAlert) {
                frameSet = &enemyAlertFrames;
                animSpeed = 5.0F;
            }

            if (!frameSet->empty()) {
                const int frameIndex = static_cast<int>(std::abs(enemy.animPhase * animSpeed))
                    % static_cast<int>(frameSet->size());
                sf::Sprite enemySprite((*frameSet)[frameIndex]);
                const sf::Vector2u texSize = (*frameSet)[frameIndex].getSize();
                if (texSize.x > 0 && texSize.y > 0) {
                    enemySprite.setOrigin(sf::Vector2f(static_cast<float>(texSize.x) / 2.F, static_cast<float>(texSize.y) / 2.F));
                    enemySprite.setPosition(sf::Vector2f(screenX + screenShakeX + headTilt * 0.35F, baseY + walkSwing));
                    enemySprite.setColor(sf::Color(255, 255, 255, 235));
                    enemySprite.setScale(sf::Vector2f(bodyHeight / static_cast<float>(texSize.y), bodyHeight / static_cast<float>(texSize.y)));
                    window.draw(enemySprite);
                    continue;
                }
            }
        }

        // Fallback, якщо спрайти не завантажені.
        sf::RectangleShape body(sf::Vector2f(bodyWidth, bodyHeight * 0.63F));
        body.setOrigin(body.getGeometricCenter());
        body.setPosition(sf::Vector2f(screenX + screenShakeX, baseY + walkSwing));
        body.setFillColor(sf::Color(7, 7, 7));
        window.draw(body);

        sf::CircleShape head(bodyWidth * 0.34F);
        head.setOrigin(head.getGeometricCenter());
        head.setPosition(sf::Vector2f(screenX + screenShakeX + headTilt, baseY - bodyHeight * 0.40F + walkSwing));
        head.setFillColor(sf::Color::Black);
        window.draw(head);

        sf::RectangleShape armL(sf::Vector2f(bodyWidth * 0.18F, bodyHeight * 0.34F));
        armL.setOrigin(armL.getGeometricCenter());
        armL.setPosition(sf::Vector2f(screenX - bodyWidth * 0.40F + screenShakeX, baseY - bodyHeight * 0.04F + walkSwing));
        armL.setRotation(sf::degrees(-25.0F + std::sin(enemy.animPhase) * 35.0F));
        armL.setFillColor(sf::Color(10, 10, 10));
        window.draw(armL);

        sf::RectangleShape armR(sf::Vector2f(bodyWidth * 0.18F, bodyHeight * 0.34F));
        armR.setOrigin(armR.getGeometricCenter());
        armR.setPosition(sf::Vector2f(screenX + bodyWidth * 0.40F + screenShakeX, baseY - bodyHeight * 0.04F + walkSwing));
        armR.setRotation(sf::degrees(25.0F - std::sin(enemy.animPhase) * 35.0F));
        armR.setFillColor(sf::Color(10, 10, 10));
        window.draw(armR);
    }
}

void LabyrinthGame::drawMiniMap() {
    constexpr float miniTile = 12.F;
    constexpr int visionRadiusCells = 6;
    const sf::Vector2f center(118.F, 118.F);
    const float circleRadius = miniTile * static_cast<float>(visionRadiusCells) + 4.F;

    sf::CircleShape bg(circleRadius);
    bg.setOrigin(bg.getGeometricCenter());
    bg.setPosition(center);
    bg.setFillColor(sf::Color(10, 10, 15, 210));
    bg.setOutlineThickness(2.F);
    bg.setOutlineColor(sf::Color(150, 150, 160));
    window.draw(bg);

    sf::RectangleShape tile(sf::Vector2f(miniTile - 1.F, miniTile - 1.F));
    const int px = static_cast<int>(player.x);
    const int py = static_cast<int>(player.y);

    for (int dy = -visionRadiusCells; dy <= visionRadiusCells; ++dy) {
        for (int dx = -visionRadiusCells; dx <= visionRadiusCells; ++dx) {
            const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist > static_cast<float>(visionRadiusCells)) {
                continue;
            }

            const int mx = px + dx;
            const int my = py + dy;
            if (!isInsideMap(mx, my)) {
                continue;
            }

            char t = map[my][mx];
            if (t >= '1' && t <= '3') {
                // На мінікарті ключ видно, тільки якщо гравець вже близько (revealed).
                bool revealed = false;
                for (const auto& key : keys) {
                    if (key.position == sf::Vector2i{mx, my}) {
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

            tile.setPosition(sf::Vector2f(center.x + dx * miniTile, center.y + dy * miniTile));
            window.draw(tile);
        }
    }

    sf::CircleShape p(4.F);
    p.setFillColor(sf::Color::Cyan);
    p.setOrigin(p.getGeometricCenter());
    p.setPosition(center);
    window.draw(p);

    sf::Vertex line[2];
    line[0].position = center;
    line[0].color = sf::Color::Cyan;
    line[1].position = sf::Vector2f(center.x + std::cos(playerAngle) * 20.F, center.y + std::sin(playerAngle) * 20.F);
    line[1].color = sf::Color::Cyan;
    window.draw(line, 2, sf::PrimitiveType::Lines);

    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) {
            continue;
        }
        const int ex = static_cast<int>(enemy.position.x);
        const int ey = static_cast<int>(enemy.position.y);
        const int dx = ex - px;
        const int dy = ey - py;
        const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
        if (dist > static_cast<float>(visionRadiusCells)) {
            continue;
        }

        sf::CircleShape e(3.8F);
        e.setOrigin(e.getGeometricCenter());
        e.setPosition(sf::Vector2f(center.x + static_cast<float>(dx) * miniTile, center.y + static_cast<float>(dy) * miniTile));
        e.setFillColor(sf::Color::Black);
        e.setOutlineThickness(1.F);
        e.setOutlineColor(sf::Color(120, 120, 120));
        window.draw(e);
    }
}

void LabyrinthGame::drawFullMapOverlay() {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);

    const float maxW = screenWidth * 0.78F;
    const float maxH = screenHeight * 0.84F;
    const float tileSize = std::min(maxW / static_cast<float>(mapWidth), maxH / static_cast<float>(mapHeight));

    const float startX = (static_cast<float>(screenWidth) - tileSize * mapWidth) / 2.F;
    const float startY = (static_cast<float>(screenHeight) - tileSize * mapHeight) / 2.F;

    sf::RectangleShape tile(sf::Vector2f(tileSize - 0.2F, tileSize - 0.2F));
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            char t = map[y][x];
            if (t == '#') {
                tile.setFillColor(sf::Color(34, 34, 40));
            } else if (t == 'D') {
                tile.setFillColor(sf::Color(120, 70, 30));
            } else if (t >= '1' && t <= '3') {
                // На повній карті ключі теж не показуємо.
                tile.setFillColor(sf::Color(165, 165, 170));
            } else if (t == 'E') {
                tile.setFillColor(sf::Color(155, 70, 220));
            } else {
                tile.setFillColor(sf::Color(165, 165, 170));
            }

            tile.setPosition(sf::Vector2f(startX + x * tileSize, startY + y * tileSize));
            window.draw(tile);
        }
    }

    sf::CircleShape playerDot(std::max(2.0F, tileSize * 0.35F));
    playerDot.setFillColor(sf::Color::Cyan);
    playerDot.setOrigin(playerDot.getGeometricCenter());
    playerDot.setPosition(sf::Vector2f(startX + player.x * tileSize, startY + player.y * tileSize));
    window.draw(playerDot);

    sf::Vertex fullMapLine[2];
    fullMapLine[0].position = sf::Vector2f(startX + player.x * tileSize, startY + player.y * tileSize);
    fullMapLine[0].color = sf::Color::Cyan;
    fullMapLine[1].position = sf::Vector2f(
        startX + (player.x + std::cos(playerAngle) * 2.0F) * tileSize,
        startY + (player.y + std::sin(playerAngle) * 2.0F) * tileSize
    );
    fullMapLine[1].color = sf::Color::Cyan;
    window.draw(fullMapLine, 2, sf::PrimitiveType::Lines);

    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) {
            continue;
        }

        sf::CircleShape marker(std::max(2.2F, tileSize * 0.30F));
        marker.setOrigin(marker.getGeometricCenter());
        marker.setPosition(sf::Vector2f(startX + enemy.position.x * tileSize, startY + enemy.position.y * tileSize));
        marker.setFillColor(sf::Color::Black);
        marker.setOutlineThickness(1.F);
        marker.setOutlineColor(sf::Color(110, 110, 120));
        window.draw(marker);
    }

    if (fontLoaded) {
        sf::Text title(font, "ПОВНА КАРТА (M - закрити)", 24);
        title.setFillColor(sf::Color::White);
        title.setPosition(sf::Vector2f(startX, startY - 34.F));
        window.draw(title);
    }
}

void LabyrinthGame::drawHud() {
    constexpr float panelW = 220.F;
    constexpr float panelH = 74.F;
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
        // Навіть без шрифту малюємо вертикальні колонки HP/витривалості.
    } else {
        sf::Text corner(font);
        corner.setCharacterSize(18);
        corner.setFillColor(sf::Color(220, 220, 230));
        corner.setPosition(sf::Vector2f(panelX + 16.F, panelY + 10.F));
        corner.setString("КЛЮЧІ: " + std::to_string(score) + "/3");
        window.draw(corner);

        sf::Text controls(font);
        controls.setCharacterSize(18);
        controls.setFillColor(sf::Color::White);
        controls.setPosition(sf::Vector2f(10.F, static_cast<float>(screenHeight - 52)));
        controls.setString("W/S - рух, A/D - поворот, Shift - біг, Shift+Space - стрибок-політ, M - карта, ESC - вихід");
        window.draw(controls);
    }

    const float columnX = 22.F;
    const float columnBottom = static_cast<float>(screenHeight) - 24.F;
    const float barW = 20.F;
    const float barH = 160.F;
    const float gap = 14.F;

    const auto drawVerticalBar = [this, columnBottom, barW, barH](float x, float ratio, const sf::Color& fill) {
        sf::RectangleShape frame(sf::Vector2f(barW, barH));
        frame.setPosition(sf::Vector2f(x, columnBottom - barH));
        frame.setFillColor(sf::Color(16, 16, 20, 220));
        frame.setOutlineThickness(2.F);
        frame.setOutlineColor(sf::Color(210, 210, 220));
        window.draw(frame);

        const float clampedRatio = std::clamp(ratio, 0.0F, 1.0F);
        const float valueH = (barH - 4.F) * clampedRatio;
        sf::RectangleShape fillRect(sf::Vector2f(barW - 4.F, valueH));
        fillRect.setPosition(sf::Vector2f(x + 2.F, columnBottom - 2.F - valueH));
        fillRect.setFillColor(fill);
        window.draw(fillRect);
    };

    drawVerticalBar(columnX, hp / 100.0F, sf::Color(220, 40, 40));
    drawVerticalBar(columnX + barW + gap, stamina / 100.0F, sf::Color(45, 120, 255));

    if (fontLoaded) {
        sf::Text hpMark(font, "+", 34);
        hpMark.setFillColor(sf::Color(245, 50, 50));
        hpMark.setPosition(sf::Vector2f(columnX + 2.F, columnBottom + 4.F));
        window.draw(hpMark);

        sf::Text stMark(font, "S", 22);
        stMark.setFillColor(sf::Color(90, 150, 255));
        stMark.setPosition(sf::Vector2f(columnX + barW + gap + 3.F, columnBottom + 10.F));
        window.draw(stMark);
    }
}

void LabyrinthGame::drawPortalScreen() {
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

void LabyrinthGame::render() {
    window.clear();

    if (gameWon) {
        drawPortalScreen();
    } else {
        drawFirstPersonWorld();
        drawMiniMap();
        drawHud();
        if (showFullMap) {
            drawFullMapOverlay();
        }
    }

    window.display();
}