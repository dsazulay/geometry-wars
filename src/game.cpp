#include "game.h"
#include "components.h"

constexpr f32 PLAYER_SPEED = 3.0f;
constexpr f32 DRAG_SPEED = 0.03f;
constexpr f32 BREAK_BOOST = 10.0f;
constexpr glm::vec2 MAX_SPEED = { 200.0f, 200.0f };

auto Game::init() -> void
{
    player.transform.pos({ 640.0f, 320.0f, -0.1f });
    player.transform.scale({ 36.0f, 36.0f });
    player.acceleration = { 0.0, 0.0 };
    player.velocity = { 0.0, 0.0 };
}

auto Game::update(f32 dt) -> void
{
    player.velocity += player.acceleration;
    player.velocity.x = glm::max(glm::min(player.velocity.x, MAX_SPEED.x), -MAX_SPEED.x);
    player.velocity.y = glm::max(glm::min(player.velocity.y, MAX_SPEED.y), -MAX_SPEED.y);
    decelerate();

    player.velocity *= checkOutOfBounds(player.transform);

    glm::vec2 pos = player.transform.pos();
    player.transform.pos(pos + player.velocity * dt);
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

auto Game::decelerate() -> void
{
    if (player.acceleration.x < 0.001f && player.acceleration.x > -0.001f)
    {
        player.velocity.x -= player.velocity.x * DRAG_SPEED;
        if (player.velocity.x > 0.0)
        {
            player.velocity.x = glm::max(player.velocity.x, 0.0f);
        }
        else if (player.velocity.x < 0.0)
        {
            player.velocity.x = glm::min(player.velocity.x, 0.0f);
        }
    }


    if (player.acceleration.y < 0.001f && player.acceleration.y > -0.001f)
    {
        player.velocity.y -= player.velocity.y * DRAG_SPEED;
        if (player.velocity.y > 0.0)
        {
            player.velocity.y = glm::max(player.velocity.y, 0.0f);
        }
        else if (player.velocity.y < 0.0)
        {
            player.velocity.y = glm::min(player.velocity.y, 0.0f);
        }
    }
}

auto Game::checkOutOfBounds(Transform& transform) -> glm::vec2
{
    glm::vec2 halfWidth = transform.scale() * 0.5f;
    glm::vec2 AA = transform.pos() - halfWidth;
    glm::vec2 BB = transform.pos() + halfWidth;

    if (AA.x <= 0 || BB.x >= gameWidth)
        return { -1.0f, 1.0f };
    else if (AA.y <= 0 || BB.y >= gameHeight)
        return { 1.0f, -1.0 };

    return { 1.0f, 1.0f };
}
