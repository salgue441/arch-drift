#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace kart {

enum class Error : int {
    Unknown = 1,
    WindowInitFailed,
    VulkanUnavailable,
    NoSuitableGpu,
    DeviceCreationFailed,
    SwapchainFailed,
    PipelineFailed,
    ShaderLoadFailed,
    OutOfDateSwapchain,
};

[[nodiscard]] constexpr std::string_view to_string(Error e) noexcept {
    switch (e) {
    case Error::Unknown:
        return "unknown error";
    case Error::WindowInitFailed:
        return "window initialization failed";
    case Error::VulkanUnavailable:
        return "Vulkan unavailable";
    case Error::NoSuitableGpu:
        return "no suitable GPU";
    case Error::DeviceCreationFailed:
        return "logical device creation failed";
    case Error::SwapchainFailed:
        return "swapchain creation failed";
    case Error::PipelineFailed:
        return "pipeline creation failed";
    case Error::ShaderLoadFailed:
        return "shader load failed";
    case Error::OutOfDateSwapchain:
        return "swapchain out of date";
    }
    return "unknown error";
}

template <typename T>
using Result = std::expected<T, Error>;

using VoidResult = std::expected<void, Error>;

inline VoidResult Ok() noexcept { return {}; }

template <typename T>
[[nodiscard]] Result<T> Unexpected(Error e) noexcept {
    return std::unexpected(e);
}

}  // namespace kart
