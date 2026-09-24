#include "game.h"

constexpr f32 PLAYER_SPEED = 1.0f;
constexpr f32 DRAG_SPEED = 0.05f;

auto Game::init() -> void
{
    player.transform.pos({ 640.0f, 320.0f, -0.1f });
    player.transform.scale({ 36.0f, 36.0f });
    player.acceleration = { 0.0, 0.0 };
}

auto Game::update() -> void
{
    decelerate();

    glm::vec2 pos = player.transform.pos();
    player.transform.pos(pos + player.acceleration);
}

auto Game::accelerate(glm::vec2 dir) -> void
{
    player.acceleration += dir * PLAYER_SPEED;
}

auto Game::shoot(glm::vec2 dir) -> void
{
}

auto Game::decelerate() -> void
{
    if (player.acceleration.x > 0.0)
    {
        player.acceleration.x -= player.acceleration.x * DRAG_SPEED;
        player.acceleration.x = glm::max(player.acceleration.x, 0.0f);
    }
    else if (player.acceleration.x < 0.0)
    {
        player.acceleration.x -= player.acceleration.x * DRAG_SPEED;
        player.acceleration.x = glm::min(player.acceleration.x, 0.0f);
    }

    if (player.acceleration.y > 0.0)
    {
        player.acceleration.y -= player.acceleration.y * DRAG_SPEED;
        player.acceleration.y = glm::max(player.acceleration.y, 0.0f);
    }
    else if (player.acceleration.y < 0.0)
    {
        player.acceleration.y -= player.acceleration.y * DRAG_SPEED;
        player.acceleration.y = glm::min(player.acceleration.y, 0.0f);
    }
}
