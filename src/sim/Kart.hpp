#pragma once

#include "input/KartInput.hpp"
#include "sim/Tunables.hpp"

#include <glm/glm.hpp>

/// @file Kart.hpp
/// @brief Arcade kart state and fixed-step integration (Phase 1: drive).

namespace kart::sim {

struct KartState {
    glm::vec3 position{0.0F, 0.0F, 0.0F};
    float yaw = 0.0F;    ///< Radians; 0 looks down -Z.
    float speed = 0.0F;  ///< Signed along forward (positive = forward).
    glm::vec3 color{0.90F, 0.22F, 0.18F};
};

class Kart {
public:
    explicit Kart(KartState state = {}, KartTunables tunables = {})
        : state_(state), tunables_(tunables) {}

    /// Integrates throttle/brake/steer and snaps to ground height/normal.
    void tick(float dt, const input::KartInput& in, float ground_y, const glm::vec3& ground_n);

    [[nodiscard]] KartState& state() noexcept { return state_; }
    [[nodiscard]] const KartState& state() const noexcept { return state_; }
    [[nodiscard]] glm::vec3 forward() const noexcept;

private:
    KartState state_;
    KartTunables tunables_;
};

}  // namespace kart::sim
