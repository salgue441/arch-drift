#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>

/// @file Result.hpp
/// @brief Recoverable error codes and Result alias used across Archdrift layers.

namespace kart {

/// Recoverable failure domains for platform and render setup.
/// @note Values are stable for logging; do not serialize them as a long-term ABI.
enum class Error : int {
    Unknown = 1,           ///< Unclassified failure.
    WindowInitFailed,      ///< GLFW init or window creation failed.
    VulkanUnavailable,     ///< Instance/extensions/loader unavailable.
    NoSuitableGpu,         ///< No physical device met queue/swapchain needs.
    DeviceCreationFailed,  ///< Logical device, queues, or command resources failed.
    SwapchainFailed,       ///< Swapchain, views, or framebuffers failed.
    PipelineFailed,        ///< Render pass or graphics pipeline failed.
    ShaderLoadFailed,      ///< SPIR-V missing or VkShaderModule creation failed.
    OutOfDateSwapchain,    ///< Surface changed; swapchain must be recreated.
};

/// Human-readable message for logs and CLI diagnostics.
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

/// Fallible value: either `T` or `Error`.
template <typename T>
using Result = std::expected<T, Error>;

/// Fallible void operation.
using VoidResult = std::expected<void, Error>;

/// Success for void operations.
inline VoidResult Ok() noexcept { return {}; }

/// Failure helper that preserves `T` in the Result type.
template <typename T>
[[nodiscard]] Result<T> Unexpected(Error e) noexcept {
    return std::unexpected(e);
}

}  // namespace kart
