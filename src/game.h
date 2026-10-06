#pragma once

#include "components.h"
#include "base/types.h"
#include "base/circular_array.h"

#include <glm/vec2.hpp>

struct Player
{
    Transform transform;
    glm::vec2 acceleration;
    glm::vec2 velocity;
    glm::vec2 reboundVelocity;
};

struct Projectile 
{
    Transform transform;
    glm::vec2 dir;
};

class Game
{
public:
    auto init() -> void;
    auto update(f32 dt, bool isMouseOnScreen, glm::vec2 mouseDir) -> void;
    auto accelerate(glm::vec2 dir) -> void;
    auto shoot(glm::vec2 dir) -> void;

    Player player;
    CircularArray<Projectile, 8> projectiles;
    i32 activeProjectiles;

    f32 gameWidth;
    f32 gameHeight;
private:
    auto decelerate(glm::vec2& vel, bool checkAcc) -> void;
    auto checkOutOfBounds(Transform& transform) -> glm::vec2;

    f32 timeSinceLastShot;
};
