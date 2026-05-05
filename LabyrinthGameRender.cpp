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

void LabyrinthGame::drawScreamer() {
    if (!screamerActive) return;

    const float flyInEnd = 0.18F;
    const float holdEnd  = 0.55F;
    const float totalDur = screamerShowDuration;
    float scale = 1.0F, alpha = 255.0F;

    if (screamerShowTimer < flyInEnd) {
        scale = 0.05F + (screamerShowTimer / flyInEnd) * 0.95F;
    } else if (screamerShowTimer >= holdEnd) {
        alpha = 255.0F * (1.0F - (screamerShowTimer - holdEnd) / (totalDur - holdEnd));
    }

    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);
    const uint8_t a = static_cast<uint8_t>(std::clamp(alpha, 0.F, 255.F));

    if (screamerLoaded) {
        sf::Sprite sp(screamerTexture);
        const sf::Vector2u ts = screamerTexture.getSize();
        if (ts.x > 0 && ts.y > 0) {
            const float fitScale = std::max(sw / ts.x, sh / ts.y) * scale;
            sp.setOrigin(sf::Vector2f(ts.x / 2.F, ts.y / 2.F));
            sp.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F));
            sp.setScale(sf::Vector2f(fitScale, fitScale));
            sp.setColor(sf::Color(255, 255, 255, a));
            window.draw(sp);
        }
    } else {
        sf::RectangleShape rect(sf::Vector2f(sw * scale, sh * scale));
        rect.setOrigin(sf::Vector2f(sw * scale / 2.F, sh * scale / 2.F));
        rect.setPosition(sf::Vector2f(sw / 2.F, sh / 2.F));
        rect.setFillColor(sf::Color(200, 0, 0, a));
        window.draw(rect);
        if (fontLoaded && scale > 0.5F) {
            sf::Text boo(font, "BOO!", 120);
            boo.setStyle(sf::Text::Bold);
            boo.setFillColor(sf::Color(255, 255, 255, a));
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

sf::Color LabyrinthGame::makeSimpleWallColor(float dist, char hitTile) const {
    if (hitTile == 'D') {
        const int rust = std::max(10, 58 - (int)(dist * 5.F));
        return sf::Color(rust, rust/2, rust/3);
    }
    const float light = std::clamp(1.0F - (dist / maxDepth), 0.0F, 1.0F);
    const int tone = (int)(light * 2.6F);
    return sf::Color(4 + tone*8, 5 + tone*9, 7 + tone*10);
}

// =====================================================================
//  РЕНДЕР — СВІТ
// =====================================================================

void LabyrinthGame::drawFirstPersonWorld() {
    const float sprintZoom  = 0.08F * sprintVisualTimer;
    const float jumpZoom    = 0.16F * flightVisualTimer;
    const float zoomRatio   = std::clamp(1.0F - sprintZoom - jumpZoom, 0.74F, 1.0F);
    const float dynamicFov  = fov * zoomRatio;
    const float horizonY    = static_cast<float>(screenHeight) / 2.F + cameraBobOffset;
    const float screenShakeX = std::sin(walkWavePhase * 1.25F)
        * (3.0F + sprintVisualTimer * 6.0F + flightVisualTimer * 9.0F);

    std::vector<float> wallDistances(screenWidth, maxDepth);

    sf::RectangleShape sky(sf::Vector2f(static_cast<float>(screenWidth), horizonY));
    sky.setFillColor(sf::Color(18, 18, 22));
    window.draw(sky);

    sf::RectangleShape ground(sf::Vector2f(static_cast<float>(screenWidth),
                               static_cast<float>(screenHeight) - horizonY));
    ground.setPosition(sf::Vector2f(0.F, horizonY));
    ground.setFillColor(sf::Color(28, 28, 32));
    window.draw(ground);

    sf::RectangleShape strip;
    for (unsigned int x = 0; x < screenWidth; ++x) {
        const float rayAngle = (playerAngle - dynamicFov / 2.F)
            + (static_cast<float>(x) / static_cast<float>(screenWidth)) * dynamicFov;
        const sf::Vector2f rayDir{std::cos(rayAngle), std::sin(rayAngle)};
        float distanceToWall = 0.F;
        char  hitTile = '.';
        while (distanceToWall < maxDepth) {
            distanceToWall += 0.03F;
            const int tx = (int)(player.x + rayDir.x * distanceToWall);
            const int ty = (int)(player.y + rayDir.y * distanceToWall);
            if (!isInsideMap(tx, ty)) { distanceToWall = maxDepth; break; }
            const char tile = tileAt(tx, ty);
            if (isBlockingTile(tile)) { hitTile = tile; break; }
        }
        const float corrDist = std::max(0.001F,
            distanceToWall * std::cos(rayAngle - playerAngle));
        wallDistances[x] = corrDist;
        const int wallH   = (int)(static_cast<float>(screenHeight) / corrDist);
        const int ceiling = std::max(0, (int)horizonY - wallH/2);
        const int floor   = std::min((int)screenHeight, ceiling + wallH);
        strip.setPosition(sf::Vector2f(static_cast<float>(x) + screenShakeX,
                                       static_cast<float>(ceiling)));
        strip.setSize(sf::Vector2f(1.F, static_cast<float>(std::max(0, floor-ceiling))));
        sf::Color wallColor = makeSimpleWallColor(distanceToWall, hitTile);
        if (hasFlashlight && flashlightBeamStrength > 0.01F) {
            float rayDiff = rayAngle - playerAngle;
            while (rayDiff >  3.14159F) rayDiff -= 6.28318F;
            while (rayDiff < -3.14159F) rayDiff += 6.28318F;
            const float angleNorm = std::abs(rayDiff) / (dynamicFov * 0.33F);
            const float cone = std::clamp(1.0F - angleNorm, 0.0F, 1.0F);
            const float distBoost = std::clamp(1.0F - corrDist / 11.5F, 0.0F, 1.0F);
            const float lightBoost = cone * distBoost * flashlightBeamStrength;
            const int add = static_cast<int>(160.0F * lightBoost);
            wallColor.r = static_cast<uint8_t>(std::min(255, wallColor.r + add));
            wallColor.g = static_cast<uint8_t>(std::min(255, wallColor.g + add));
            wallColor.b = static_cast<uint8_t>(std::min(255, wallColor.b + add));
        }
        strip.setFillColor(wallColor);
        window.draw(strip);
    }

    // NPC
    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) continue;
        const sf::Vector2f toEnemy = enemy.position - player;
        const float dist = std::sqrt(toEnemy.x*toEnemy.x + toEnemy.y*toEnemy.y);
        if (dist < 0.1F || dist >= maxDepth) continue;
        float angleToEnemy = std::atan2(toEnemy.y, toEnemy.x) - playerAngle;
        while (angleToEnemy >  3.14159F) angleToEnemy -= 6.28318F;
        while (angleToEnemy < -3.14159F) angleToEnemy += 6.28318F;
        if (std::abs(angleToEnemy) > dynamicFov * 0.60F) continue;
        const float screenX = ((angleToEnemy + dynamicFov/2.0F) / dynamicFov)
                              * static_cast<float>(screenWidth);
        const int columnX = (int)screenX;
        if (columnX < 0 || columnX >= (int)screenWidth) continue;
        if (dist > wallDistances[columnX]) continue;

        const float attackScale = (enemy.state == EnemyAttack) ? 1.25F : 1.0F;
        const float bodyHeight  = (static_cast<float>(screenHeight) / dist) * 0.95F * attackScale;
        const float bodyWidth   = std::max(8.0F, bodyHeight * 0.30F);
        const float walkSwing   = std::sin(enemy.animPhase) *
            (enemy.state == EnemyWander || enemy.state == EnemyChase ? 6.5F : 2.0F);
        const float headTilt    = (enemy.state == EnemyAlert)
                                  ? std::sin(enemy.animPhase*0.8F)*9.0F : 0.0F;
        const float baseY       = horizonY + cameraBobOffset * 0.12F;

        if (enemySpriteAssetsLoaded) {
            const std::vector<sf::Texture>* fs = &enemyWalkFrames;
            float animSpeed = 8.0F;
            if (enemy.state == EnemyAttack)     { fs = &enemyAttackFrames; animSpeed = 12.0F; }
            else if (enemy.state == EnemyAlert) { fs = &enemyAlertFrames;  animSpeed =  5.0F; }
            if (!fs->empty()) {
                const int fi = (int)std::abs(enemy.animPhase * animSpeed) % (int)fs->size();
                sf::Sprite sp((*fs)[fi]);
                const sf::Vector2u ts = (*fs)[fi].getSize();
                if (ts.x > 0 && ts.y > 0) {
                    sp.setOrigin(sf::Vector2f(ts.x/2.F, ts.y/2.F));
                    sp.setPosition(sf::Vector2f(screenX+screenShakeX+headTilt*0.35F, baseY+walkSwing));
                    sp.setColor(sf::Color(255,255,255,235));
                    sp.setScale(sf::Vector2f(bodyHeight/ts.y, bodyHeight/ts.y));
                    window.draw(sp); continue;
                }
            }
        }

        sf::RectangleShape body(sf::Vector2f(bodyWidth, bodyHeight*0.63F));
        body.setOrigin(body.getGeometricCenter());
        body.setPosition(sf::Vector2f(screenX+screenShakeX, baseY+walkSwing));
        body.setFillColor(sf::Color(7,7,7)); window.draw(body);

        sf::CircleShape head(bodyWidth*0.34F);
        head.setOrigin(head.getGeometricCenter());
        head.setPosition(sf::Vector2f(screenX+screenShakeX+headTilt, baseY-bodyHeight*0.40F+walkSwing));
        head.setFillColor(sf::Color::Black); window.draw(head);

        sf::RectangleShape armL(sf::Vector2f(bodyWidth*0.18F, bodyHeight*0.34F));
        armL.setOrigin(armL.getGeometricCenter());
        armL.setPosition(sf::Vector2f(screenX-bodyWidth*0.40F+screenShakeX, baseY-bodyHeight*0.04F+walkSwing));
        armL.setRotation(sf::degrees(-25.0F+std::sin(enemy.animPhase)*35.0F));
        armL.setFillColor(sf::Color(10,10,10)); window.draw(armL);

        sf::RectangleShape armR(sf::Vector2f(bodyWidth*0.18F, bodyHeight*0.34F));
        armR.setOrigin(armR.getGeometricCenter());
        armR.setPosition(sf::Vector2f(screenX+bodyWidth*0.40F+screenShakeX, baseY-bodyHeight*0.04F+walkSwing));
        armR.setRotation(sf::degrees(25.0F-std::sin(enemy.animPhase)*35.0F));
        armR.setFillColor(sf::Color(10,10,10)); window.draw(armR);
    }
   // Предмети (ліхтарик і ніж)
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            const char tile = map[y][x];
            if (tile != 'F' && tile != 'K') continue;

            const sf::Vector2f itemPos{static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F};
            const sf::Vector2f toItem = itemPos - player;
            const float dist = std::sqrt(toItem.x * toItem.x + toItem.y * toItem.y);
            if (dist < 0.1F || dist >= maxDepth) continue;
            if (!hasLineOfSight(player, itemPos, 0.05F)) continue;

            float angleToItem = std::atan2(toItem.y, toItem.x) - playerAngle;
            while (angleToItem >  3.14159F) angleToItem -= 6.28318F;
            while (angleToItem < -3.14159F) angleToItem += 6.28318F;
            if (std::abs(angleToItem) > dynamicFov * 0.60F) continue;

            const float screenX = ((angleToItem + dynamicFov / 2.0F) / dynamicFov)
                                  * static_cast<float>(screenWidth);
            const int columnX = static_cast<int>(screenX);
            if (columnX < 0 || columnX >= static_cast<int>(screenWidth)) continue;
            if (dist > wallDistances[columnX]) continue;

            const float spriteHeight = std::max(12.0F, (static_cast<float>(screenHeight) / dist) * 0.28F);
            const float screenY = horizonY + spriteHeight * 0.82F;
            const sf::Color tint = sf::Color(255, 255, 255, 245);

            if (tile == 'F' && flashlightItemLoaded) {
                sf::Sprite item(flashlightItemTexture);
                const sf::Vector2u ts = flashlightItemTexture.getSize();
                if (ts.x > 0 && ts.y > 0) {
                    const float scale = spriteHeight / static_cast<float>(ts.y);
                    item.setOrigin(sf::Vector2f(ts.x / 2.F, ts.y / 2.F));
                    item.setPosition(sf::Vector2f(screenX + screenShakeX, screenY));
                    item.setScale(sf::Vector2f(scale, scale));
                    item.setColor(tint);
                    window.draw(item);
                    continue;
                }
            }

            if (tile == 'K' && knifeItemLoaded) {
                sf::Sprite item(knifeItemTexture);
                const sf::Vector2u ts = knifeItemTexture.getSize();
                if (ts.x > 0 && ts.y > 0) {
                    const float scale = spriteHeight / static_cast<float>(ts.y);
                    item.setOrigin(sf::Vector2f(ts.x / 2.F, ts.y / 2.F));
                    item.setPosition(sf::Vector2f(screenX + screenShakeX, screenY));
                    item.setScale(sf::Vector2f(scale, scale));
                    item.setColor(tint);
                    window.draw(item);
                    continue;
                }
            }

            // якщо текстура не завантажилась — залишаємо тільки коробочку
        }
    }
}

// =====================================================================
//  РУКИ ГРАВЦЯ — єдина версія (зі спрайтом + fallback)
// =====================================================================

void LabyrinthGame::drawPlayerHands(bool isWalking, bool isSprinting) {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);
    const float swingAngle  = isWalking ? 8.0F : 1.5F;
    const float sprintShift = sprintVisualTimer * 28.0F;

    if (handsLoaded) {
        sf::Sprite hands(handsTexture);
        const sf::Vector2u ts = handsTexture.getSize();
        if (ts.x > 0 && ts.y > 0) {
            const float s = std::max(sw / static_cast<float>(ts.x),
                                     sh / static_cast<float>(ts.y));
            hands.setScale(sf::Vector2f(s, s));
            hands.setOrigin(sf::Vector2f(ts.x / 2.F, ts.y / 2.F));
            hands.setPosition(sf::Vector2f(
                sw / 2.F + handBobX * 1.5F,
                sh / 2.F + handBobY + sprintShift + sh * 0.18F));
            hands.setRotation(sf::degrees(
                std::sin(handSwayPhase) * (isWalking ? swingAngle * 0.4F : 0.5F)));
            window.draw(hands);
        }
    } else {
        // Fallback — прямокутники
        const float handW = sw * 0.13F;
        const float handH = sh * 0.42F;
        const float baseLeftX  = sw * 0.08F + handBobX;
        const float baseRightX = sw * 0.92F - handBobX;
        const float baseY      = sh * 0.72F + handBobY;
        const float sprintOffX = sprintVisualTimer * 14.0F;
        const float sprintOffY = sprintVisualTimer * 28.0F;
        const sf::Color skin  (38, 28, 22);
        const sf::Color sleeve(22, 18, 14);

        sf::RectangleShape leftArm(sf::Vector2f(handW*0.55F, handH*0.72F));
        leftArm.setFillColor(sleeve);
        leftArm.setOrigin(sf::Vector2f(leftArm.getSize().x*0.5F, 0.F));
        leftArm.setPosition({baseLeftX-sprintOffX, baseY+sprintOffY});
        leftArm.setRotation(sf::degrees(-12.F+std::sin(handSwayPhase)*swingAngle));
        window.draw(leftArm);

        sf::RectangleShape leftHand(sf::Vector2f(handW*0.52F, handH*0.22F));
        leftHand.setFillColor(skin);
        leftHand.setOrigin(sf::Vector2f(leftHand.getSize().x*0.5F, 0.F));
        const float lRad = sf::degrees(-12.F+std::sin(handSwayPhase)*swingAngle).asRadians();
        leftHand.setPosition({baseLeftX-sprintOffX + std::sin(lRad)*leftArm.getSize().y*0.5F,
                               baseY+sprintOffY + std::cos(lRad)*leftArm.getSize().y});
        leftHand.setRotation(sf::degrees(-12.F+std::sin(handSwayPhase)*swingAngle));
        window.draw(leftHand);

        sf::RectangleShape rightArm(sf::Vector2f(handW*0.55F, handH*0.72F));
        rightArm.setFillColor(sleeve);
        rightArm.setOrigin(sf::Vector2f(rightArm.getSize().x*0.5F, 0.F));
        rightArm.setPosition({baseRightX+sprintOffX, baseY+sprintOffY});
        rightArm.setRotation(sf::degrees(12.F-std::sin(handSwayPhase)*swingAngle));
        window.draw(rightArm);

        sf::RectangleShape rightHand(sf::Vector2f(handW*0.52F, handH*0.22F));
        rightHand.setFillColor(skin);
        rightHand.setOrigin(sf::Vector2f(rightHand.getSize().x*0.5F, 0.F));
        const float rRad = sf::degrees(12.F-std::sin(handSwayPhase)*swingAngle).asRadians();
        rightHand.setPosition({baseRightX+sprintOffX + std::sin(rRad)*rightArm.getSize().y*0.5F,
                                baseY+sprintOffY + std::cos(rRad)*rightArm.getSize().y});
        rightHand.setRotation(sf::degrees(12.F-std::sin(handSwayPhase)*swingAngle));
        window.draw(rightHand);
    }
    (void)isSprinting;
}

// =====================================================================
//  ВІНЬЄТКА
// =====================================================================

void LabyrinthGame::drawVignette() {
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);
    const sf::Vector2f center(sw / 2.F, sh / 2.F);
    constexpr int segments = 64;

    sf::VertexArray vignette(sf::PrimitiveType::TriangleFan, segments + 2);
    vignette[0].position = center;
    vignette[0].color    = sf::Color(0, 0, 0, 0);
    for (int i = 0; i <= segments; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(segments)
                            * 2.0F * 3.14159265F;
        vignette[i+1].position = sf::Vector2f(
            center.x + std::cos(angle) * sw * 0.72F,
            center.y + std::sin(angle) * sh * 0.72F);
        vignette[i+1].color = sf::Color(0, 0, 0, 232);
    }
    window.draw(vignette);

    if (ambientDarknessAlpha > 1.0F) {
        sf::RectangleShape dim(sf::Vector2f(sw, sh));
        dim.setFillColor(sf::Color(0, 0, 0,
            static_cast<uint8_t>(ambientDarknessAlpha)));
        window.draw(dim);
    if (hasFlashlight) {
        constexpr int beamSegments = 56;
        sf::VertexArray beam(sf::PrimitiveType::TriangleFan, beamSegments + 2);
        const sf::Vector2f beamCenter(sw / 2.F, sh * 0.60F);
        const uint8_t beamCenterAlpha = static_cast<uint8_t>(
            std::clamp(52.0F - flashlightBeamStrength * 40.0F, 8.0F, 70.0F));
        const uint8_t beamEdgeAlpha = static_cast<uint8_t>(
            std::clamp(248.0F - flashlightBeamStrength * 55.0F, 165.0F, 252.0F));
        beam[0].position = beamCenter;
        beam[0].color = sf::Color(0, 0, 0, beamCenterAlpha);
        for (int i = 0; i <= beamSegments; ++i) {
            const float a = -0.95F + (1.90F * static_cast<float>(i) / beamSegments);
            const float rx = std::sin(a) * sw * 0.58F;
            const float ry = std::cos(a) * sh * 0.72F;
            beam[i + 1].position = {beamCenter.x + rx, beamCenter.y - ry};
            beam[i + 1].color = sf::Color(0, 0, 0, beamEdgeAlpha);
        }
        window.draw(beam);
    }
    }

       if (hasFlashlight) {
        constexpr int beamSegments = 56;
        sf::VertexArray beam(sf::PrimitiveType::TriangleFan, beamSegments + 2);
        const sf::Vector2f beamCenter(sw / 2.F, sh * 0.60F);
        const uint8_t beamCenterAlpha = static_cast<uint8_t>(
            std::clamp(52.0F - flashlightBeamStrength * 40.0F, 8.0F, 70.0F));
        const uint8_t beamEdgeAlpha = static_cast<uint8_t>(
            std::clamp(248.0F - flashlightBeamStrength * 55.0F, 165.0F, 252.0F));
        beam[0].position = beamCenter;
        beam[0].color = sf::Color(0, 0, 0, beamCenterAlpha);
        for (int i = 0; i <= beamSegments; ++i) {
            const float a = -0.95F + (1.90F * static_cast<float>(i) / beamSegments);
            const float rx = std::sin(a) * sw * 0.58F;
            const float ry = std::cos(a) * sh * 0.72F;
            beam[i + 1].position = {beamCenter.x + rx, beamCenter.y - ry};
            beam[i + 1].color = sf::Color(0, 0, 0, beamEdgeAlpha);
        }
        window.draw(beam);
    }

    if (hp < 55.0F) {
        const float pulse = std::abs(std::sin(flickerPhase * 2.5F));
        const uint8_t redAlpha = static_cast<uint8_t>(
            std::clamp((55.0F - hp) / 55.0F * (90.0F + pulse * 120.0F), 0.0F, 220.0F));
        sf::RectangleShape damageFx(sf::Vector2f(sw, sh));
        damageFx.setFillColor(sf::Color(170, 0, 0, redAlpha));
        window.draw(damageFx);
    }
}

// =====================================================================
//  МІНІКАРТА
// =====================================================================

void LabyrinthGame::drawMiniMap() {
    constexpr float miniTile = 12.F;
    constexpr int   visionR  = 6;
    const sf::Vector2f center(118.F, 118.F);
    const float circleR = miniTile * visionR + 4.F;

    sf::CircleShape bg(circleR);
    bg.setOrigin(bg.getGeometricCenter()); bg.setPosition(center);
    bg.setFillColor(sf::Color(10,10,15,210));
    bg.setOutlineThickness(2.F); bg.setOutlineColor(sf::Color(150,150,160));
    window.draw(bg);

    sf::RectangleShape tile(sf::Vector2f(miniTile-1.F, miniTile-1.F));
    const int px = (int)player.x, py = (int)player.y;

    for (int dy = -visionR; dy <= visionR; ++dy)
        for (int dx = -visionR; dx <= visionR; ++dx) {
            if (std::sqrt((float)(dx*dx+dy*dy)) > visionR) continue;
            const int mx = px+dx, my = py+dy;
            if (!isInsideMap(mx, my)) continue;
            char t = map[my][mx];
            if (t >= '1' && t <= '3') {
                bool rev = false;
                for (const auto& k : keys)
                    if (k.position == sf::Vector2i{mx,my}) { rev=k.revealed; break; }
                if (!rev) t = '.';
            }
            if      (t == '#') tile.setFillColor(sf::Color(42,42,48));
            else if (t == 'D') tile.setFillColor(sf::Color(120,70,30));
            else if (t == 'F') tile.setFillColor(sf::Color(235, 235, 160));
            else if (t == 'K') tile.setFillColor(sf::Color(200, 200, 220));
            else if (t >= '1' && t <= '3') tile.setFillColor(sf::Color::Yellow);
            else if (t == 'E') tile.setFillColor(sf::Color(155,70,220));
            else               tile.setFillColor(sf::Color(180,180,180));
            tile.setPosition(sf::Vector2f(center.x+dx*miniTile, center.y+dy*miniTile));
            window.draw(tile);
        }

    sf::CircleShape p(4.F);
    p.setFillColor(sf::Color::Cyan); p.setOrigin(p.getGeometricCenter()); p.setPosition(center);
    window.draw(p);

    sf::Vertex line[2];
    line[0].position = center; line[0].color = sf::Color::Cyan;
    line[1].position = {center.x+std::cos(playerAngle)*20.F, center.y+std::sin(playerAngle)*20.F};
    line[1].color = sf::Color::Cyan;
    window.draw(line, 2, sf::PrimitiveType::Lines);

    for (const auto& enemy : enemies) {
        if (!enemy.visibleToPlayer) continue;
        const int dx = (int)enemy.position.x - px;
        const int dy = (int)enemy.position.y - py;
        if (std::sqrt((float)(dx*dx+dy*dy)) > visionR) continue;
        sf::CircleShape e(3.8F);
        e.setOrigin(e.getGeometricCenter());
        e.setPosition({center.x+dx*miniTile, center.y+dy*miniTile});
        e.setFillColor(sf::Color::Black);
        e.setOutlineThickness(1.F); e.setOutlineColor(sf::Color(120,120,120));
        window.draw(e);
    }
}

// =====================================================================
//  ПОВНА КАРТА
// =====================================================================

void LabyrinthGame::drawFullMapOverlay() {
    sf::RectangleShape dim(sf::Vector2f((float)screenWidth, (float)screenHeight));
    dim.setFillColor(sf::Color(0,0,0,150)); window.draw(dim);

    const float ts = std::min(screenWidth*0.78F/mapWidth, screenHeight*0.84F/mapHeight);
    const float sx = ((float)screenWidth  - ts*mapWidth)  / 2.F;
    const float sy = ((float)screenHeight - ts*mapHeight) / 2.F;

    sf::RectangleShape tile(sf::Vector2f(ts-0.2F, ts-0.2F));
    for (int y = 0; y < mapHeight; ++y)
        for (int x = 0; x < mapWidth; ++x) {
            const char t = map[y][x];
            if      (t == '#') tile.setFillColor(sf::Color(34,34,40));
            else if (t == 'D') tile.setFillColor(sf::Color(120,70,30));
            else if (t == 'F') tile.setFillColor(sf::Color(235, 235, 160));
            else if (t == 'K') tile.setFillColor(sf::Color(200, 200, 220));
            else if (t == 'E') tile.setFillColor(sf::Color(155,70,220));
            else               tile.setFillColor(sf::Color(165,165,170));
            tile.setPosition(sf::Vector2f(sx+x*ts, sy+y*ts));
            window.draw(tile);
        }

    sf::CircleShape pd(std::max(2.0F, ts*0.35F));
    pd.setFillColor(sf::Color::Cyan); pd.setOrigin(pd.getGeometricCenter());
    pd.setPosition({sx+player.x*ts, sy+player.y*ts}); window.draw(pd);

    sf::Vertex fl[2];
    fl[0].position = {sx+player.x*ts, sy+player.y*ts}; fl[0].color = sf::Color::Cyan;
    fl[1].position = {sx+(player.x+std::cos(playerAngle)*2.F)*ts,
                      sy+(player.y+std::sin(playerAngle)*2.F)*ts}; fl[1].color = sf::Color::Cyan;
    window.draw(fl, 2, sf::PrimitiveType::Lines);

    for (const auto& e : enemies) {
        if (!e.visibleToPlayer) continue;
        sf::CircleShape m(std::max(2.2F, ts*0.30F));
        m.setOrigin(m.getGeometricCenter());
        m.setPosition({sx+e.position.x*ts, sy+e.position.y*ts});
        m.setFillColor(sf::Color::Black);
        m.setOutlineThickness(1.F); m.setOutlineColor(sf::Color(110,110,120));
        window.draw(m);
    }

    if (fontLoaded) {
        sf::Text title(font, utf8("Повна карта (M - закрити)"), 24);
        title.setFillColor(sf::Color::White);
        title.setPosition(sf::Vector2f(sx, sy-34.F));
        window.draw(title);
    }
}

// =====================================================================
//  HUD
// =====================================================================

void LabyrinthGame::drawHud() {
    constexpr float panelW = 220.F, panelH = 74.F;
    const float panelX = (float)screenWidth - panelW - 16.F, panelY = 16.F;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(8,8,12,220));
    panel.setOutlineThickness(1.5F); panel.setOutlineColor(sf::Color(90,90,105));
    window.draw(panel);

    for (int i = 0; i < 3; ++i) {
        sf::CircleShape orb(10.F);
        orb.setOrigin(orb.getGeometricCenter());
        orb.setPosition({panelX+30.F+i*30.F, panelY+panelH/2.F+10.F});
        orb.setFillColor(i < score ? sf::Color(235,205,70) : sf::Color(55,55,60));
        orb.setOutlineThickness(1.F); orb.setOutlineColor(sf::Color(130,130,140));
        window.draw(orb);
    }

    const float colX = 22.F, colBot = (float)(screenHeight-24);
    const float barW = 20.F, barH = 160.F, gap = 14.F;
    const auto drawBar = [&](float x, float ratio, sf::Color fill) {
        sf::RectangleShape frame(sf::Vector2f(barW, barH));
        frame.setPosition({x, colBot-barH});
        frame.setFillColor(sf::Color(16,16,20,220));
        frame.setOutlineThickness(10.F); frame.setOutlineColor(sf::Color(210,210,220));
        window.draw(frame);
        const float vh = (barH-1.F)*std::clamp(ratio,0.F,1.F);
        sf::RectangleShape fr(sf::Vector2f(barW-4.F, vh));
        fr.setPosition({x+2.F, colBot-2.F-vh}); fr.setFillColor(fill);
        window.draw(fr);
    };
    drawBar(colX,          hp/100.F,      sf::Color(220,40,40));
    drawBar(colX+barW+gap, stamina/100.F, sf::Color(45,120,255));

    (void)fontLoaded;
}

// =====================================================================
//  GAME OVER
// =====================================================================

void LabyrinthGame::drawGameOver() {
    const float sw = (float)screenWidth, sh = (float)screenHeight;
    sf::RectangleShape bg(sf::Vector2f(sw, sh));
    bg.setFillColor(sf::Color(0,0,0)); window.draw(bg);

    const float pulse = std::abs(std::sin(portalClock.getElapsedTime().asSeconds() * 1.2F));
    sf::CircleShape glow(280.F);
    glow.setOrigin(glow.getGeometricCenter());
    glow.setPosition({sw/2.F, sh/2.F});
    glow.setFillColor(sf::Color(80,0,0, (uint8_t)(60.F+pulse*60.F)));
    window.draw(glow);

    if (fontLoaded) {
        sf::Text title(font, utf8("Ви програли"), 90);
        title.setStyle(sf::Text::Bold); title.setFillColor(sf::Color(200,20,20));
        const auto tb = title.getLocalBounds();
        title.setOrigin({tb.size.x/2.F, tb.size.y/2.F});
        title.setPosition({sw/2.F, sh/2.F-60.F}); window.draw(title);

        sf::Text sub(font, utf8("Спробуйте ще раз"), 30);
        sub.setFillColor(sf::Color(160,160,160));
        const auto sb = sub.getLocalBounds();
        sub.setOrigin({sb.size.x/2.F, sb.size.y/2.F});
        sub.setPosition({sw/2.F, sh/2.F+30.F}); window.draw(sub);
    }

    window.draw(restartButton.box);
    window.draw(menuButton.box);
    if (restartButton.label) window.draw(*restartButton.label);
    if (menuButton.label) window.draw(*menuButton.label);
}

// =====================================================================
//  ЕКРАН ПЕРЕМОГИ
// =====================================================================

void LabyrinthGame::drawVictoryScreen() {
    sf::RectangleShape bg(sf::Vector2f((float)screenWidth, (float)screenHeight));
    bg.setFillColor(sf::Color(8, 14, 28));
    window.draw(bg);

    const float t = portalClock.getElapsedTime().asSeconds();
    const sf::Vector2f center{screenWidth / 2.F, screenHeight / 2.F - 70.F};
    for (int i = 0; i < 7; ++i) {
        const float radius = 40.F + i * 32.F + std::sin(t * 1.7F + i * 0.5F) * 8.F;
        sf::CircleShape ring(radius);
        ring.setOrigin(ring.getGeometricCenter());
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.F);
        ring.setOutlineColor(sf::Color(90 + i * 14, 130 + i * 12, 240 - i * 14, 220));
        window.draw(ring);
    }

    if (fontLoaded) {
        sf::Text title(font, utf8("Ти переміг"), 78);
        title.setStyle(sf::Text::Bold);
        const float progress = std::clamp(victoryScreenTimer / 2.4F, 0.0F, 1.0F);
        const uint8_t titleAlpha = static_cast<uint8_t>(255.0F * progress);
        title.setFillColor(sf::Color(255, 255, 255, titleAlpha));
        const auto tb = title.getLocalBounds();
        title.setOrigin({tb.size.x / 2.F, tb.size.y / 2.F});
        title.setPosition({screenWidth / 2.F, screenHeight / 2.F + 30.F});
        window.draw(title);
    }

}

void LabyrinthGame::drawTransitionOverlay() {
    if (transitionAlpha <= 0.0F) return;
    sf::RectangleShape overlay(sf::Vector2f((float)screenWidth, (float)screenHeight));
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(std::clamp(transitionAlpha, 0.0F, 255.0F))));
    window.draw(overlay);
}

// =====================================================================
//  ПОРТАЛ
// =====================================================================

void LabyrinthGame::drawPortalScreen() {
    sf::RectangleShape bg(sf::Vector2f((float)screenWidth, (float)screenHeight));
    bg.setFillColor(sf::Color(8,8,20)); window.draw(bg);

    const float t = portalClock.getElapsedTime().asSeconds();
    const sf::Vector2f center{screenWidth/2.F, screenHeight/2.F-50.F};
    for (int i = 0; i < 8; ++i) {
        const float radius = 30.F + i*26.F + std::sin(t*2.F+i)*6.F;
        sf::CircleShape ring(radius);
        ring.setOrigin(ring.getGeometricCenter()); ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.F);
        ring.setOutlineColor(sf::Color(95+i*18, 40+i*17, 220-i*15));
        window.draw(ring);
    }
    if (!fontLoaded) return;
        sf::Text title(font, utf8("Систему зламано. Ти вільний"), 42);
        title.setStyle(sf::Text::Bold); title.setFillColor(sf::Color::White);
        title.setPosition({120.F, (float)screenHeight-170.F}); window.draw(title);
        sf::Text tip(font, utf8("Натисни Esc, щоб закрити гру"), 26);
        tip.setFillColor(sf::Color(220,220,230));
        tip.setPosition({300.F, (float)screenHeight-115.F}); window.draw(tip);
}

// =====================================================================
//  РЕНДЕР
// =====================================================================
