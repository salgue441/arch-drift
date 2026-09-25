/// @file Kart.cpp
/// @brief Phase 1 arcade drive model (accel / brake / speed-scaled steer).

#include "sim/Kart.hpp"

#include <algorithm>
#include <cmath>

namespace kart::sim {

glm::vec3 Kart::forward() const noexcept {
    return {std::sin(state_.yaw), 0.0F, -std::cos(state_.yaw)};
}

void Kart::tick(float dt, const input::KartInput& in, float ground_y, const glm::vec3& ground_n) {
    (void)ground_n;

    const float speed_ratio =
        std::clamp(std::abs(state_.speed) / std::max(tunables_.max_speed, 1.0F), 0.0F, 1.0F);
    const float steer_scale = 1.0F - tunables_.steer_speed_falloff * speed_ratio;
    state_.yaw += in.steer * tunables_.steer_rate * steer_scale * dt;

    if (in.throttle > 0.0F) {
        state_.speed += tunables_.accel * in.throttle * dt;
    } else if (in.brake > 0.0F) {
        if (state_.speed > 0.0F) {
            state_.speed -= tunables_.brake_decel * in.brake * dt;
            state_.speed = std::max(state_.speed, 0.0F);
        } else {
            state_.speed -= tunables_.accel * 0.45F * in.brake * dt;
        }
    } else {
        if (state_.speed > 0.0F) {
            state_.speed = std::max(0.0F, state_.speed - tunables_.coast_decel * dt);
        } else if (state_.speed < 0.0F) {
            state_.speed = std::min(0.0F, state_.speed + tunables_.coast_decel * dt);
        }
    }

    state_.speed = std::clamp(state_.speed, -tunables_.max_speed * 0.35F, tunables_.max_speed);

    const glm::vec3 fwd = forward();
    state_.position += fwd * (state_.speed * dt);
    state_.position.y = ground_y;
}

}  // namespace kart::sim
