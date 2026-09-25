#pragma once

#include "core/Result.hpp"

#include <cstdint>
#include <string>
#include <utility>

struct GLFWwindow;

/// @file Window.hpp
/// @brief GLFW window RAII wrapper configured for Vulkan (no OpenGL context).

namespace kart::platform {

/// Parameters for Window::create.
struct WindowConfig {
    int width = 1280;              ///< Client width in screen coordinates.
    int height = 720;              ///< Client height in screen coordinates.
    std::string title = "Archdrift";  ///< Window title bar text.
};

/// Owns a GLFW window suitable for creating a VkSurfaceKHR.
///
/// @thread_safety Main thread only (GLFW constraint).
/// @note Non-copyable, movable. After a move, call attach_callbacks() on the
///       destination object so framebuffer resize callbacks bind to the new address.
class Window {
public:
    Window() = default;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    ~Window();

    /// Creates a Vulkan-ready window (`GLFW_NO_API`).
    /// @return Owning window, or WindowInitFailed / VulkanUnavailable.
    [[nodiscard]] static Result<Window> create(const WindowConfig& config);

    /// Registers framebuffer-resize callbacks using `this` as the user pointer.
    /// @pre Must be called once the Window object lives at its stable address
    ///      (after any move into long-lived storage).
    void attach_callbacks() noexcept;

    /// Native GLFW handle for surface creation and input queries.
    [[nodiscard]] GLFWwindow* handle() const noexcept { return window_; }

    /// True when the user requested close (or Escape set the flag in main).
    [[nodiscard]] bool should_close() const noexcept;

    /// Pumps OS events; call once per frame before sampling input.
    void poll_events() const noexcept;

    /// Drawable size in pixels (may differ from WindowConfig on HiDPI).
    [[nodiscard]] std::pair<int, int> framebuffer_size() const noexcept;

    /// Latched resize flag for swapchain recreation; cleared by clear_resized().
    [[nodiscard]] bool was_resized() noexcept;

    /// Clears the latched resize flag after the renderer recreates the swapchain.
    void clear_resized() noexcept { resized_ = false; }

    /// @param glfw_key GLFW key token (e.g. GLFW_KEY_ESCAPE).
    /// @return True while the key is held.
    [[nodiscard]] bool key_pressed(int glfw_key) const noexcept;

private:
    explicit Window(GLFWwindow* window) noexcept;

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

    GLFWwindow* window_ = nullptr;
    bool resized_ = false;
};

}  // namespace kart::platform
