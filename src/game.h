#pragma once

#include "components.h"
#include "base/types.h"

#include <glm/vec2.hpp>

struct Player
{
    Transform transform;
    glm::vec2 acceleration;
    glm::vec2 velocity;
};

class Game
{
public:
    auto init() -> void;
    auto update(f32 dt) -> void;
    auto accelerate(glm::vec2 dir) -> void;
    auto shoot(glm::vec2 dir) -> void;

    Player player;
    f32 gameWidth;
    f32 gameHeight;
private:
    auto decelerate() -> void;
    auto checkOutOfBounds(Transform& transform) -> glm::vec2;

};
