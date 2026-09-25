/// @file Camera.cpp
/// @brief Chase-camera matrices for Phase 1.

#include "sim/Camera.hpp"

#include <cmath>

namespace kart::sim {

glm::mat4 Camera::view_matrix(const KartState& kart) const noexcept {
    const glm::vec3 forward{std::sin(kart.yaw), 0.0F, -std::cos(kart.yaw)};
    const glm::vec3 eye = kart.position - forward * distance + glm::vec3{0.0F, height, 0.0F};
    const glm::vec3 target = kart.position + forward * look_ahead + glm::vec3{0.0F, 1.0F, 0.0F};
    return glm::lookAt(eye, target, glm::vec3{0.0F, 1.0F, 0.0F});
}

glm::mat4 Camera::proj_matrix(float aspect) const noexcept {
    return glm::perspective(glm::radians(fov_y_degrees), aspect, near_plane, far_plane);
}

}  // namespace kart::sim
