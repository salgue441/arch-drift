#pragma once

#include <string>

/// @file Tunables.hpp
/// @brief Arcade handling constants (defaults + optional JSON overrides).

namespace kart::sim {

struct KartTunables {
    float max_speed = 28.0F;
    float accel = 18.0F;
    float brake_decel = 32.0F;
    float coast_decel = 8.0F;
    float steer_rate = 2.4F;
    float steer_speed_falloff = 0.55F;
};

/// Loads known float fields from a minimal JSON object, or returns defaults.
[[nodiscard]] KartTunables load_tunables_or_default(const std::string& path);

}  // namespace kart::sim
