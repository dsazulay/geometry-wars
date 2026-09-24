#pragma once

#include "components.h"
#include "base/types.h"

#include <glm/vec2.hpp>

struct Player
{
    Transform transform;
    glm::vec2 acceleration;
};

class Game
{
public:
    auto init() -> void;
    auto update() -> void;
    auto accelerate(glm::vec2 dir) -> void;
    auto shoot(glm::vec2 dir) -> void;

    Player player;
private:
    auto decelerate() -> void;
};
