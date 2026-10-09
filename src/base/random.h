#pragma once

#include "types.h"

#include <random>

class Random
{
public:
    static auto init() -> void;
    static auto randFloat() -> f32;
    static auto randInt(i32 max) -> i32;
private:
    static std::uniform_real_distribution<f32> s_distribution;
    static std::default_random_engine s_randomEngine;
};
