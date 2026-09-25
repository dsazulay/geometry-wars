#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Transform
{
    auto pos(glm::vec3 p) -> void
    {
        pos_ = p;
        updateModel();
    }

    auto pos(glm::vec2 p) -> void
    {
        pos_ = glm::vec3{ p, 0.0 };
        updateModel();
    }

    auto pos() -> glm::vec2
    {
        return { pos_.x, pos_.y };
    }

    auto posZ(float z) -> void
    {
        pos_.z = z;
        updateModel();
    }

    auto scale(glm::vec2 s) -> void
    {
        model_[0][0] = s.x;
        model_[1][1] = s.y;
    }

    auto scale() -> glm::vec2
    {
        return { model_[0][0], model_[1][1] };
    }

    auto model() -> glm::mat4
    {
        return model_;
    }

private:
    auto updateModel() -> void
    {
        model_[3] = glm::vec4{ pos_, 1.0 };
    }

    glm::vec3 pos_{ 0.0 };
    glm::mat4 model_{ 1.0 };
};

