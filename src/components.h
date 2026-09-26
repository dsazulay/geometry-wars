#pragma once

#include "base/types.h"
#include "glm/ext/matrix_transform.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform
{
    auto pos(glm::vec3 p) -> void
    {
        pos_ = p;
        updateModel();
    }

    auto pos(glm::vec2 p) -> void
    {
        pos_ = glm::vec3{ p, 0.0f };
        updateModel();
    }

    auto pos() -> glm::vec2
    {
        return { pos_.x, pos_.y };
    }

    auto posZ(f32 z) -> void
    {
        pos_.z = z;
        updateModel();
    }

    auto angle(f32 angle) -> void
    {
        angle_ = angle;
    }

    auto angle() -> f32
    {
        return angle_;
    }

    auto scale(glm::vec2 s) -> void
    {
        scale_ = glm::vec3{ s, 1.0f };
    }

    auto scale() -> glm::vec2
    {
        return { scale_.x, scale_.y };
    }

    auto model() -> glm::mat4
    {
        return model_;
    }

private:
    auto updateModel() -> void
    {
        model_ = glm::translate(glm::mat4{ 1.0f }, pos_);
        model_ = glm::rotate(model_, angle_, glm::vec3{ 0.0f, 0.0f, 1.0f });
        model_ = glm::scale(model_, scale_);
    }

    glm::vec3 pos_{ 0.0f };
    f32 angle_{ 0.0f };
    glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };
    glm::mat4 model_{ 1.0f };
};

