/// @file KeyboardSource.cpp
/// @brief GLFW key polling for local play.

#include "input/KeyboardSource.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace kart::input {

KartInput KeyboardSource::sample() const noexcept {
    KartInput in{};
    if (window_ == nullptr) {
        return in;
    }

    const bool up = window_->key_pressed(GLFW_KEY_W) || window_->key_pressed(GLFW_KEY_UP);
    const bool down = window_->key_pressed(GLFW_KEY_S) || window_->key_pressed(GLFW_KEY_DOWN);
    const bool left = window_->key_pressed(GLFW_KEY_A) || window_->key_pressed(GLFW_KEY_LEFT);
    const bool right = window_->key_pressed(GLFW_KEY_D) || window_->key_pressed(GLFW_KEY_RIGHT);

    in.throttle = up ? 1.0F : 0.0F;
    in.brake = down ? 1.0F : 0.0F;
    in.steer = 0.0F;
    if (left) {
        in.steer -= 1.0F;
    }
    if (right) {
        in.steer += 1.0F;
    }
    in.steer = std::clamp(in.steer, -1.0F, 1.0F);
    in.drift = window_->key_pressed(GLFW_KEY_LEFT_SHIFT) || window_->key_pressed(GLFW_KEY_RIGHT_SHIFT);
    in.item = window_->key_pressed(GLFW_KEY_SPACE);
    return in;
}

}  // namespace kart::input
