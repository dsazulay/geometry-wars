#include "random.h"

std::uniform_real_distribution<f32> Random::s_distribution(0.0, 1.0);
std::default_random_engine Random::s_randomEngine;

auto Random::init() -> void
{
    s_randomEngine.seed(std::random_device()());
}

auto Random::randFloat() -> f32
{
    return s_distribution(s_randomEngine);
}

auto Random::randInt(i32 max) -> i32
{
    return (i32) (randFloat() * max);
}
