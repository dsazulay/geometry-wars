#include "game.h"

#include "components.h"
#include "base/random.h"
#include "glm/geometric.hpp"
#include <cmath>

constexpr f32 PLAYER_SPEED = 4.0f;
constexpr f32 DRAG_SPEED = 0.03f;
constexpr f32 BREAK_BOOST = 10.0f;
constexpr f32 MAX_SPEED = 200.0f;

constexpr f32 SHOOT_FREQ = 0.25f;
constexpr f32 PROJECTILE_VELOCITY = 400.0f;

constexpr f32 ENEMY_SIZE = 75.0f;
constexpr f32 ENEMY_HALFSIZE = ENEMY_SIZE / 2.0f;
constexpr f32 SPAWN_RATE = 2.0f;
constexpr u32 MAX_ENEMY_TYPE = 3;

auto Game::init() -> void
{
    player.transform.scale({ 36.0f, 36.0f });
    player.transform.pos({ 0.0f, 0.0f, 0.0f });
    player.body.acceleration = { 0.0, 0.0 };
    player.body.velocity = { 0.0, 0.0 };
    player.body.reboundVelocity = { 0.0, 0.0 };

    activeProjectiles = 0;
    timeSinceLastShot = 0.0f;

    timeSinceLastSpawn = 0.0f;

    deleteQueue.reserve(8);
    enemyDeleteQueue.reserve(10);
}

auto Game::update(f32 dt, bool isMouseOnScreen, glm::vec2 mouseDir, glm::vec2 inputDir) -> void
{
    accelerate(player.body, inputDir);
    player.body.velocity += player.body.acceleration;

    f32 speed = glm::length(player.body.velocity);
    if (speed > MAX_SPEED)
    {
        player.body.velocity = (player.body.velocity / speed) * MAX_SPEED;
    }

    player.body.reboundVelocity += player.body.velocity * checkOutOfBounds(player.transform);

    f32 angle = atan2(-player.body.velocity.y, -player.body.velocity.x);
    player.transform.angle(angle);

    glm::vec2 pos = player.transform.pos();
    player.transform.pos(pos + (player.body.velocity + player.body.reboundVelocity) * dt);

    decelerate(player.body.velocity, true);
    decelerate(player.body.reboundVelocity, false);

    player.body.acceleration = { 0.0f, 0.0f };

    for (auto& e : enemies)
    {
        accelerate(e.body, player.transform.pos() - e.transform.pos());
        e.body.velocity += e.body.acceleration;
        speed = glm::length(e.body.velocity);
        if (speed > MAX_SPEED)
        {
            e.body.velocity = (e.body.velocity / speed) * MAX_SPEED;
        }

        e.body.reboundVelocity += e.body.velocity * checkOutOfBounds(e.transform);

        pos = e.transform.pos();
        e.transform.pos(pos + (e.body.velocity + e.body.reboundVelocity) * dt);

        decelerate(e.body.velocity, false);
        decelerate(e.body.reboundVelocity, false);

        e.body.acceleration = { 0.0f, 0.0f };
    }

    timeSinceLastShot += dt;
    if (timeSinceLastShot >= SHOOT_FREQ && isMouseOnScreen)
    {
        shoot(mouseDir);
        timeSinceLastShot = 0.0f;
    }

    timeSinceLastSpawn += dt;
    if (timeSinceLastSpawn >= SPAWN_RATE)
    {
        spawn();
        timeSinceLastSpawn = 0.0f;
    }

    for (u32 i = 0; i < projectiles.size(); ++i)
    {
        auto& p = projectiles[i];
        p.transform.pos(p.transform.pos() + p.dir * PROJECTILE_VELOCITY * dt);
        glm::vec2 outOfBound = checkOutOfBounds(p.transform);
        if (outOfBound.x < 0.0f || outOfBound.y < 0.0f)
        {
            deleteQueue.push_back(i);
            continue;
        }

        for (u32 j = 0; j < enemies.size(); ++j)
        {
            auto& e = enemies[j];
            if (checkCollision(p.transform, e.transform))
            {
                deleteQueue.push_back(i);
                enemyDeleteQueue.push_back(j);
            }
        }
    }

    for (u32 i : deleteQueue)
    {
        projectiles.swap_remove_at(i);
    }

    for (u32 i : enemyDeleteQueue)
    {
        enemies.swap_remove_at(i);
    }

    deleteQueue.clear();
    enemyDeleteQueue.clear();
}

auto Game::accelerate(Body& body, glm::vec2 dir) -> void
{
    if (glm::length(dir) <= 0.0f) return;

    body.acceleration += glm::normalize(dir) * PLAYER_SPEED;

    if ((body.velocity.x > 0.0f && dir.x < 0.0f) ||
        (body.velocity.x < 0.0f && dir.x > 0.0f))
    {
        body.acceleration.x *= BREAK_BOOST;
    }

    if ((body.velocity.y > 0.0f && dir.y < 0.0f) ||
        (body.velocity.y < 0.0f && dir.y > 0.0f))
    {
        body.acceleration.y *= BREAK_BOOST;
    }
}

auto Game::shoot(glm::vec2 dir) -> void
{
    // TODO: check when mouse is on 0, 0
    glm::vec2 shootDir = glm::normalize(dir - glm::vec2{640.0f, 360.0f} - player.transform.pos());
    Transform t;
    t.scale({ 20.0f, 20.0f });
    t.pos(player.transform.pos());
    projectiles.push_back({ t, shootDir });
}

auto Game::spawn() -> void
{
    i32 x = Random::randInt(gameWidth - ENEMY_SIZE) - (gameWidth / 2.0f - ENEMY_HALFSIZE);
    i32 y = Random::randInt(gameHeight - ENEMY_SIZE) - (gameHeight/ 2.0f - ENEMY_HALFSIZE);
    Enemy::Type type = (Enemy::Type) Random::randInt(MAX_ENEMY_TYPE);
    Transform t;
    t.scale({ ENEMY_SIZE, ENEMY_SIZE });
    t.pos({ x, y });
    enemies.push_back({ t, {}, type });
}

auto Game::decelerate(glm::vec2& vel, bool checkAcc) -> void
{
    if (!checkAcc || (player.body.acceleration.x < 0.001f && player.body.acceleration.x > -0.001f))
    {
        if (vel.x > 0.0)
        {
            vel.x -= vel.x * DRAG_SPEED;
            vel.x = glm::max(vel.x, 0.0f);
        }
        else if (vel.x < 0.0)
        {
            vel.x -= vel.x * DRAG_SPEED;
            vel.x = glm::min(vel.x, 0.0f);
        }
    }

    if (!checkAcc || (player.body.acceleration.y < 0.001f && player.body.acceleration.y > -0.001f))
    {
        if (vel.y > 0.0)
        {
            vel.y -= vel.y * DRAG_SPEED;
            vel.y = glm::max(vel.y, 0.0f);
        }
        else if (vel.y < 0.0)
        {
            vel.y -= vel.y * DRAG_SPEED;
            vel.y = glm::min(vel.y, 0.0f);
        }
    }
}

auto Game::checkOutOfBounds(Transform& transform) -> glm::vec2
{
    glm::vec2 halfSize = transform.scale() * 0.5f;
    glm::vec2 AA = transform.pos() - halfSize;
    glm::vec2 BB = transform.pos() + halfSize;

    float halfWidth = gameWidth * 0.5f;
    float halfHeight = gameHeight * 0.5f;

    if (AA.x <= -halfWidth || BB.x >= halfWidth)
    {
        glm::vec2 pos = transform.pos();
        pos.x = glm::max(glm::min(pos.x, halfWidth - halfSize.x), -halfWidth + halfSize.x);
        transform.pos(pos);
        return { -0.5f, 0.0f };
    }
    else if (AA.y <= -halfHeight || BB.y >= halfHeight)
    {
        glm::vec2 pos = transform.pos();
        pos.y = glm::max(glm::min(pos.y, halfHeight - halfSize.y), -halfWidth + halfSize.y);
        transform.pos(pos);
        return { 0.0f, -0.5f };
    }

    return { 0.0f, 0.0f };
}

auto Game::checkCollision(Transform& a, Transform& b) -> bool
{
    glm::vec2 halfSize = a.scale() * 0.5f;
    glm::vec2 AA = a.pos() - halfSize;
    glm::vec2 BB = a.pos() + halfSize;

    halfSize = b.scale() * 0.5f;
    glm::vec2 CC = b.pos() - halfSize;
    glm::vec2 DD = b.pos() + halfSize;

    if (BB.x < CC.x) return false;
    if (AA.x > DD.x) return false;
    if (BB.y < CC.y) return false;
    if (AA.y > DD.y) return false;

    return true;
}
