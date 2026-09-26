#include "game.h"

#include "components.h"
#include <cmath>

constexpr f32 PLAYER_SPEED = 4.0f;
constexpr f32 DRAG_SPEED = 0.03f;
constexpr f32 BREAK_BOOST = 10.0f;
constexpr glm::vec2 MAX_SPEED = { 200.0f, 200.0f };

auto Game::init() -> void
{
    player.transform.scale({ 36.0f, 36.0f });
    player.transform.pos({ 640.0f, 320.0f, 0.0f });
    player.acceleration = { 0.0, 0.0 };
    player.velocity = { 0.0, 0.0 };
    player.reboundVelocity = { 0.0, 0.0 };
}

auto Game::update(f32 dt) -> void
{
    player.velocity += player.acceleration;
    player.velocity.x = glm::max(glm::min(player.velocity.x, MAX_SPEED.x), -MAX_SPEED.x);
    player.velocity.y = glm::max(glm::min(player.velocity.y, MAX_SPEED.y), -MAX_SPEED.y);

    player.reboundVelocity += player.velocity * checkOutOfBounds(player.transform);

    f32 angle = atan2(-player.velocity.y, -player.velocity.x);
    player.transform.angle(angle);

    glm::vec2 pos = player.transform.pos();
    player.transform.pos(pos + (player.velocity + player.reboundVelocity) * dt);

    decelerate(player.velocity, true);
    decelerate(player.reboundVelocity, false);

    player.acceleration = { 0.0f, 0.0f };
}

auto Game::accelerate(glm::vec2 dir) -> void
{
    player.acceleration += dir * PLAYER_SPEED;

    if ((player.velocity.x > 0.0f && dir.x < 0.0f) ||
        (player.velocity.x < 0.0f && dir.x > 0.0f))
    {
        player.acceleration.x *= BREAK_BOOST;
    }

    if ((player.velocity.y > 0.0f && dir.y < 0.0f) ||
        (player.velocity.y < 0.0f && dir.y > 0.0f))
    {
        player.acceleration.y *= BREAK_BOOST;
    }
}

auto Game::shoot(glm::vec2 dir) -> void
{
}

auto Game::decelerate(glm::vec2& vel, bool checkAcc) -> void
{
    if (!checkAcc || (player.acceleration.x < 0.001f && player.acceleration.x > -0.001f))
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

    if (!checkAcc || (player.acceleration.y < 0.001f && player.acceleration.y > -0.001f))
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

    if (AA.x <= 0 || BB.x >= gameWidth)
    {
        glm::vec2 pos = transform.pos();
        pos.x = glm::max(glm::min(pos.x, gameWidth - halfSize.x), halfSize.x);
        transform.pos(pos);
        return { -0.5f, 0.0f };
    }
    else if (AA.y <= 0 || BB.y >= gameHeight)
    {
        glm::vec2 pos = transform.pos();
        pos.y = glm::max(glm::min(pos.y, gameHeight - halfSize.y), halfSize.y);
        transform.pos(pos);
        return { 0.0f, -0.5f };
    }

    return { 0.0f, 0.0f };
}
