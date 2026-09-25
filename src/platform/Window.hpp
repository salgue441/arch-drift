#pragma once

#include "core/Result.hpp"

#include <cstdint>
#include <string>

struct GLFWwindow;

namespace kart::platform {

struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Qantara Kart";
};

class Window {
public:
    Window() = default;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    ~Window();

    [[nodiscard]] static Result<Window> create(const WindowConfig& config);

    // Must be called once the Window lives at its stable address (after move into owner).
    void attach_callbacks() noexcept;

    [[nodiscard]] GLFWwindow* handle() const noexcept { return window_; }
    [[nodiscard]] bool should_close() const noexcept;
    void poll_events() const noexcept;

    [[nodiscard]] std::pair<int, int> framebuffer_size() const noexcept;
    [[nodiscard]] bool was_resized() noexcept;
    void clear_resized() noexcept { resized_ = false; }

    [[nodiscard]] bool key_pressed(int glfw_key) const noexcept;

private:
    explicit Window(GLFWwindow* window) noexcept;

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

    GLFWwindow* window_ = nullptr;
    bool resized_ = false;
};

}  // namespace kart::platform
