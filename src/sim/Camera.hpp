#pragma once

#include "sim/Kart.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/// @file Camera.hpp
/// @brief Chase camera behind the player kart.

namespace kart::sim {

struct Camera {
    float distance = 10.0F;
    float height = 4.5F;
    float look_ahead = 4.0F;
    float fov_y_degrees = 60.0F;
    float near_plane = 0.1F;
    float far_plane = 400.0F;

    [[nodiscard]] glm::mat4 view_matrix(const KartState& kart) const noexcept;
    [[nodiscard]] glm::mat4 proj_matrix(float aspect) const noexcept;
};

}  // namespace kart::sim
