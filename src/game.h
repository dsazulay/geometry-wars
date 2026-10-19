#pragma once

#include "components.h"
#include "base/types.h"
#include "base/circular_array.h"

#include <glm/vec2.hpp>
#include <vector>

constexpr u32 MAX_PROJECTILES = 10;
constexpr u32 MAX_ENEMIES = 10;

struct Body
{
    glm::vec2 acceleration;
    glm::vec2 velocity;
};

struct Player
{
    Transform transform;
    Body body;
    bool isDead;
};

struct Enemy
{
    enum class Type
    {
        Square,
        Circle,
        Diamond
    };

    Transform transform;
    Body body;
    Type type;
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
    auto update(f32 dt, bool isMouseOnScreen, glm::vec2 mouseDir, glm::vec2 inputDir) -> void;
    auto accelerate(Body& body, glm::vec2 dir) -> void;
    auto shoot(glm::vec2 dir) -> void;
    auto spawn() -> void;

    Player player;
    CircularArray<Projectile, MAX_PROJECTILES> projectiles;
    CircularArray<Enemy, MAX_ENEMIES> enemies;
    std::vector<u32> deleteQueue;
    std::vector<u32> enemyDeleteQueue;

    i32 activeProjectiles;

    f32 gameWidth;
    f32 gameHeight;
private:
    auto decelerate(glm::vec2& vel, bool checkAcc) -> void;
    auto checkOutOfBounds(Transform& transform) -> glm::vec2;
    auto checkCollision(Transform& a, Transform& b) -> bool;

    f32 timeSinceLastShot;
    f32 timeSinceLastSpawn;
};
