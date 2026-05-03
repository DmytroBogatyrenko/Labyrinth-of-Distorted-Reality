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

bool LabyrinthGame::isInsideMap(int x, int y) const {
    return x >= 0 && x < mapWidth && y >= 0 && y < mapHeight;
}

char LabyrinthGame::tileAt(int x, int y) const { return map[y][x]; }

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
            prev = idx; prevRow[tx] = idx;
        }
    }

    for (int c = 1; c < chunkGrid; ++c) {
        const int sXL = c * chunkSize - 1, sXR = c * chunkSize;
        for (int y = 3; y < mapHeight - 3; y += 6)
            for (int k = 0; k < 3 && y + k < mapHeight - 1; ++k)
                { map[y+k][sXL] = '.'; map[y+k][sXR] = '.'; }
        const int sYT = c * chunkSize - 1, sYB = c * chunkSize;
        for (int x = 3; x < mapWidth - 3; x += 6)
            for (int k = 0; k < 3 && x + k < mapWidth - 1; ++k)
                { map[sYT][x+k] = '.'; map[sYB][x+k] = '.'; }
    }

    player = sf::Vector2f(1.5F, 1.5F);
    map[1][1] = '.'; map[1][2] = '.'; map[2][1] = '.';
    map[mapHeight-4][mapWidth-4] = 'D';
    map[mapHeight-4][mapWidth-3] = 'e';
    map[mapHeight-5][mapWidth-4] = '.';
    map[mapHeight-4][mapWidth-5] = '.';
    for (int x = 1; x <= mapWidth-5; ++x) map[2][x] = '.';
    for (int y = 2; y <= mapHeight-4; ++y) map[y][mapWidth-5] = '.';
}

void LabyrinthGame::placeKeysRandomly() {
    keys.clear();
    std::vector<sf::Vector2i> candidates;
    for (int y = 1; y < mapHeight-1; ++y)
        for (int x = 1; x < mapWidth-1; ++x) {
            if (map[y][x] != '.') continue;
            if (x <= 4 && y <= 4) continue;
            if (x >= mapWidth-7 && y >= mapHeight-7) continue;
            candidates.push_back({x, y});
        }
    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);
    std::vector<sf::Vector2i> picked;
    for (const auto& pos : candidates) {
        bool ok = true;
        for (const auto& p : picked) {
            int dx = p.x-pos.x, dy = p.y-pos.y;
            if (dx*dx+dy*dy < 120) { ok=false; break; }
        }
        if (!ok) continue;
        picked.push_back(pos);
        if (picked.size() == 3) break;
    }
    for (int i = 0; i < (int)picked.size(); ++i) {
        map[picked[i].y][picked[i].x] = static_cast<char>('1'+i);
        keys.push_back(KeyInfo{picked[i], false, false});
    }
}

void LabyrinthGame::spawnEnemiesRandomly() {
    enemies.clear();
    std::vector<sf::Vector2f> candidates;
    for (int y = 2; y < mapHeight-2; ++y)
        for (int x = 2; x < mapWidth-2; ++x) {
            if (map[y][x] != '.') continue;
            if (x < 6 && y < 6) continue;
            candidates.emplace_back(x+0.5F, y+0.5F);
        }
    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);
    for (const auto& c : candidates) {
        bool ok = true;
        for (const auto& e : enemies) {
            sf::Vector2f d = e.position - c;
            if (d.x*d.x+d.y*d.y < 70.F) { ok=false; break; }
        }
        if (!ok) continue;
        EnemyInfo e; e.position=c; e.speed=1.3F; e.state=EnemyWander; e.wanderTarget=c;
        enemies.push_back(e);
        if ((int)enemies.size() >= 4) break;
    }
}

bool LabyrinthGame::loadEnemyFrameSet(const std::string& prefix, int count,
                                       std::vector<sf::Texture>& out) {
    out.clear();
    if (!std::filesystem::exists(prefix + "0.png")) return false;
    for (int i = 0; i < count; ++i) {
        sf::Texture f;
        if (!f.loadFromFile(prefix + std::to_string(i) + ".png")) return false;
        f.setSmooth(true);
        out.push_back(std::move(f));
    }
    return !out.empty();
}

void LabyrinthGame::loadEnemySpriteAssets() {
    const bool walkOk   = loadEnemyFrameSet("assets/npc/walk_",   6, enemyWalkFrames);
    const bool attackOk = loadEnemyFrameSet("assets/npc/attack_", 4, enemyAttackFrames);
    const bool alertOk  = loadEnemyFrameSet("assets/npc/alert_",  3, enemyAlertFrames);
    bool singleOk = false;
    if (!walkOk) {
        sf::Texture s;
        if (s.loadFromFile("assets/npc/model.png")) {
            s.setSmooth(true); enemyWalkFrames.push_back(std::move(s)); singleOk=true;
        }
    }
    enemySpriteAssetsLoaded = walkOk || singleOk;
    if (enemySpriteAssetsLoaded && !attackOk) enemyAttackFrames = enemyWalkFrames;
    if (enemySpriteAssetsLoaded && !alertOk)  enemyAlertFrames  = enemyWalkFrames;
    if (!enemySpriteAssetsLoaded) {
        enemyWalkFrames.clear(); enemyAttackFrames.clear(); enemyAlertFrames.clear();
        std::cerr << "[Попередження] NPC спрайти не знайдені. Fallback-силует.\n";
    }
}

// =====================================================================
//  ПОДІЇ
// =====================================================================

void LabyrinthGame::movePlayer(const sf::Vector2f& dir, float distanceStep) {
    if (dir.x == 0.F && dir.y == 0.F) return;
    const float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    const sf::Vector2f n = dir / len;
    const sf::Vector2f candidate = player + n * distanceStep;
    const int tx = (int)candidate.x, ty = (int)candidate.y;
    if (!isInsideMap(tx, ty)) return;
    if (isBlockingTile(tileAt(tx, ty))) return;
    player = candidate;
}

bool LabyrinthGame::hasLineOfSight(const sf::Vector2f& from, const sf::Vector2f& to,
                                    float step) const {
    const sf::Vector2f delta = to - from;
    const float dist = std::sqrt(delta.x*delta.x + delta.y*delta.y);
    if (dist <= 0.001F) return true;
    const sf::Vector2f dir = delta / dist;
    for (float t = 0.0F; t < dist; t += step) {
        const sf::Vector2f p = from + dir * t;
        if (!isInsideMap((int)p.x, (int)p.y)) return false;
        if (isBlockingTile(tileAt((int)p.x, (int)p.y))) return false;
    }
    return true;
}

bool LabyrinthGame::isWalkableEnemyCell(int x, int y) const {
    if (!isInsideMap(x, y)) return false;
    const char t = tileAt(x, y);
    return t == '.' || t == 'E' || t == 'F' || t == 'K' || (t >= '1' && t <= '3');
}

void LabyrinthGame::moveEnemyToward(EnemyInfo& enemy, const sf::Vector2f& target,
                                     float dt, float speedScale) {
    sf::Vector2f delta = target - enemy.position;
    const float len = std::sqrt(delta.x*delta.x + delta.y*delta.y);
    if (len <= 0.001F) return;
    sf::Vector2f candidate = enemy.position + (delta/len) * enemy.speed * speedScale * dt;
    if (isWalkableEnemyCell((int)candidate.x, (int)candidate.y))
        enemy.position = candidate;
}

sf::Vector2f LabyrinthGame::chooseEnemyWanderTarget(const sf::Vector2f& origin) const {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> offset(-6, 6);
    for (int i = 0; i < 25; ++i) {
        const int cx = (int)origin.x + offset(rng);
        const int cy = (int)origin.y + offset(rng);
        if (!isWalkableEnemyCell(cx, cy)) continue;
        if (countWallNeighbors(cx, cy) >= 7) continue;
        return sf::Vector2f(cx+0.5F, cy+0.5F);
    }
    return origin;
}

// =====================================================================
//  ПРЕДМЕТИ / ВЗАЄМОДІЯ / БІЙ
// =====================================================================

void LabyrinthGame::placeStartingItems() {
    const int flashlightX = 2;
    const int flashlightY = 1;
    const int knifeX = 4;
    const int knifeY = 2;

    if (!hasFlashlight && isInsideMap(flashlightX, flashlightY)) {
        map[flashlightY][flashlightX] = 'F';
    }
    if (!hasKnife && isInsideMap(knifeX, knifeY)) {
        map[knifeY][knifeX] = 'K';
    }
}

bool LabyrinthGame::tryLoadHandsTexture(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        if (!std::filesystem::exists(path)) continue;
        if (!handsTexture.loadFromFile(path)) continue;
        handsTexture.setSmooth(true);
        handsLoaded = true;
        handsSprite.setOrigin({ static_cast<float>(handsTexture.getSize().x) / 2.F,
                                static_cast<float>(handsTexture.getSize().y) });
        handsSprite.setPosition({ static_cast<float>(screenWidth) / 2.F,
                                  static_cast<float>(screenHeight) });
        std::cerr << "[INFO] Hands loaded: " << path << "\n";
        return true;
    }
    return false;
}

void LabyrinthGame::refreshHandsTexture() {
    bool loaded = false;
    if (hasFlashlight && hasKnife) {
        loaded = tryLoadHandsTexture({"assets/hands_flashlight_knife.png", "assets/hands__.png"});
    } else if (hasFlashlight) {
        loaded = tryLoadHandsTexture({"assets/hands_flashlight.png", "assets/hands.png"});
    } else if (hasKnife) {
        loaded = tryLoadHandsTexture({"assets/hands_knife.png", "assets/hands.png", "assets/hands_clean.png"});
    } else {
        loaded = tryLoadHandsTexture({"assets/hands_clean.png"});
    }

    if (!loaded) {
        handsLoaded = false;
        std::cerr << "[WARNING] Не знайдено підходяще зображення рук.\n";
    }
}

void LabyrinthGame::handleInteraction() {
    if (!interactionPressed) return;
    interactionPressed = false;

    bool pickedAny = false;
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            const int tx = static_cast<int>(player.x) + ox;
            const int ty = static_cast<int>(player.y) + oy;
            if (!isInsideMap(tx, ty)) continue;
            char& tile = map[ty][tx];
            if (tile == 'F' && !hasFlashlight) {
                hasFlashlight = true;
                tile = '.';
                pickedAny = true;
            } else if (tile == 'K' && !hasKnife) {
                hasKnife = true;
                tile = '.';
                pickedAny = true;
            }
        }
    }

    if (pickedAny) {
        pickupTransitionTimer = 0.22F;
        refreshHandsTexture();
        if (pickupSound.has_value()) pickupSound->play();
    }
}

void LabyrinthGame::handleCombat() {
    if (!attackPressed) return;
    attackPressed = false;
    if (!(hasFlashlight && hasKnife)) return;
        if (attackSound.has_value()) {
        attackSound->stop();
        attackSound->play();
    }

    bool enemyKilled = false;
    for (auto& enemy : enemies) {
        const sf::Vector2f toEnemy = enemy.position - player;
        const float dist = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y);
        if (dist > 1.8F) continue;

        float angleDiff = std::atan2(toEnemy.y, toEnemy.x) - playerAngle;
        while (angleDiff >  3.14159F) angleDiff -= 6.28318F;
        while (angleDiff < -3.14159F) angleDiff += 6.28318F;
        if (std::abs(angleDiff) > 0.65F) continue;
        if (!hasLineOfSight(player, enemy.position, 0.08F)) continue;

        const float prevHp = enemy.hp;
        enemy.hp = std::max(0.0F, enemy.hp - 20.0F);
        if (prevHp > 0.0F && enemy.hp <= 0.0F) enemyKilled = true;
    }

    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const EnemyInfo& enemy) { return enemy.hp <= 0.0F; }),
        enemies.end());
    if (enemyKilled && enemyDeathSound.has_value()) {
    enemyDeathSound->stop();
    enemyDeathSound->play();
    }
}


// =====================================================================
//  ВОРОГИ
// =====================================================================

void LabyrinthGame::updateEnemies(float dt) {
    for (auto& enemy : enemies) {
        enemy.attackCooldown = std::max(0.0F, enemy.attackCooldown - dt);

        const sf::Vector2f toPlayer = player - enemy.position;
        const float distToPlayer = std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y);
        const bool canSee = distToPlayer < 9.5F &&
                            hasLineOfSight(enemy.position, player, 0.10F);

        const sf::Vector2f toEnemy = enemy.position - player;
        const float ped = std::sqrt(toEnemy.x*toEnemy.x + toEnemy.y*toEnemy.y);
        float angleDiff = std::atan2(toEnemy.y, toEnemy.x) - playerAngle;
        while (angleDiff >  3.14159F) angleDiff -= 6.28318F;
        while (angleDiff < -3.14159F) angleDiff += 6.28318F;
        enemy.visibleToPlayer = ped < 13.0F
            && std::abs(angleDiff) < fov * 0.58F
            && hasLineOfSight(player, enemy.position, 0.08F);

        enemy.animPhase += dt * (enemy.state == EnemyChase ? 10.0F : 6.0F);

        if (canSee) {
            enemy.lastSeenPlayerPos = player;
            enemy.hasLastSeen = true;
            if (enemy.state == EnemyWander || enemy.state == EnemySearch) {
                enemy.state = EnemyAlert; enemy.stateTimer = 0.75F;
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
                if (enemy.stateTimer <= 0.0F) { enemy.state = EnemyChase; enemy.chasing = true; }
                break;
            case EnemyChase:
                moveEnemyToward(enemy, player, dt, 1.25F);
                if (!canSee) { enemy.state = EnemySearch; enemy.stateTimer = 2.8F; }
                if (distToPlayer < 1.15F && enemy.attackCooldown <= 0.0F) {
                    enemy.state = EnemyAttack; enemy.stateTimer = 0.30F;
                    enemy.attackCooldown = 1.2F;
                    hp = std::max(0.0F, hp - 15.0F);
                }
                break;
            case EnemySearch:
                if (enemy.hasLastSeen)
                    moveEnemyToward(enemy, enemy.lastSeenPlayerPos, dt, 1.0F);
                enemy.stateTimer -= dt;
                if (canSee) { enemy.state = EnemyAlert; enemy.stateTimer = 0.65F; }
                else if (enemy.stateTimer <= 0.0F ||
                         std::hypot(enemy.lastSeenPlayerPos.x - enemy.position.x,
                                    enemy.lastSeenPlayerPos.y - enemy.position.y) < 0.6F) {
                    enemy.state = EnemyWander; enemy.chasing = false;
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
            default: enemy.state = EnemyWander; break;
        }
    }
}

// =====================================================================
//  КЛЮЧІ / ДВЕРІ / ВИХІД
// =====================================================================

void LabyrinthGame::revealNearbyKeys() {
    const int px = (int)player.x, py = (int)player.y;
    for (auto& key : keys) {
        if (key.collected) continue;
        if (std::abs(px-key.position.x) <= 1 && std::abs(py-key.position.y) <= 1)
            key.revealed = true;
    }
}

void LabyrinthGame::collectAtPlayerCell() {
    const int px = (int)player.x, py = (int)player.y;
    char& tile = map[py][px];
    if (tile >= '1' && tile <= '3') {
        ++score;
        tile = '.';
        for (auto& key : keys)
            if (key.position == sf::Vector2i{px, py}) key.collected = true;
        if (pickupSound.has_value()) pickupSound->play();
    }
}

void LabyrinthGame::unlockDoorAndSpawnExit() {
    if (score != 3 || exitSpawned) return;
    exitSpawned = true;
    for (auto& row : map) {
        std::replace(row.begin(), row.end(), 'D', '.');
        std::replace(row.begin(), row.end(), 'e', 'E');
    }
}

void LabyrinthGame::checkWin() {
    if (map[(int)player.y][(int)player.x] == 'E') gameWon = true;
}

// =====================================================================
//  КОЛІР СТІНИ
// =====================================================================
