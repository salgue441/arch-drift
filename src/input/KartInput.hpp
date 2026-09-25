#pragma once

/// @file KartInput.hpp
/// @brief Per-tick control snapshot shared by keyboard and phone pads.

namespace kart::input {

/// Normalized controls consumed by the arcade sim (main thread).
struct KartInput {
    float throttle = 0.0F;  ///< 0..1 accelerate.
    float brake = 0.0F;     ///< 0..1 brake.
    float steer = 0.0F;     ///< -1..1 (negative = left).
    bool drift = false;     ///< Hold to enter/maintain drift (Phase 2).
    bool item = false;      ///< Item button held (Phase 4).
};

}  // namespace kart::input
