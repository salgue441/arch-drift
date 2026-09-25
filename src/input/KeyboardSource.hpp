#pragma once

#include "input/KartInput.hpp"
#include "platform/Window.hpp"

/// @file KeyboardSource.hpp
/// @brief Maps WASD / arrows / Shift into KartInput.

namespace kart::input {

class KeyboardSource {
public:
    explicit KeyboardSource(platform::Window& window) noexcept : window_(&window) {}

    /// Samples current key state into a KartInput snapshot.
    [[nodiscard]] KartInput sample() const noexcept;

private:
    platform::Window* window_ = nullptr;
};

}  // namespace kart::input
