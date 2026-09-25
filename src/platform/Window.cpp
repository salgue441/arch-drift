/// @file Window.cpp
/// @brief GLFW window implementation and process-wide glfwInit lifecycle.

#include "platform/Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <utility>

namespace kart::platform {
namespace {

struct GlfwLibrary {
    GlfwLibrary() {
        if (!glfwInit()) {
            ok_ = false;
            return;
        }
        glfwSetErrorCallback([](int code, const char* description) {
            (void)code;
            (void)description;
        });
        ok_ = true;
    }

    ~GlfwLibrary() {
        if (ok_) {
            glfwTerminate();
        }
    }

    GlfwLibrary(const GlfwLibrary&) = delete;
    GlfwLibrary& operator=(const GlfwLibrary&) = delete;

    [[nodiscard]] bool ok() const noexcept { return ok_; }

private:
    bool ok_ = false;
};

GlfwLibrary& glfw_library() {
    static GlfwLibrary library;
    return library;
}

}  // namespace

Window::Window(GLFWwindow* window) noexcept : window_(window) {}

Window::Window(Window&& other) noexcept
    : window_(std::exchange(other.window_, nullptr)), resized_(other.resized_) {}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
        }
        window_ = std::exchange(other.window_, nullptr);
        resized_ = other.resized_;
    }
    return *this;
}

Window::~Window() {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

Result<Window> Window::create(const WindowConfig& config) {
    if (!glfw_library().ok()) {
        return Unexpected<Window>(Error::WindowInitFailed);
    }

    if (glfwVulkanSupported() != GLFW_TRUE) {
        return Unexpected<Window>(Error::VulkanUnavailable);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* handle =
        glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (handle == nullptr) {
        return Unexpected<Window>(Error::WindowInitFailed);
    }

    return Window(handle);
}

void Window::attach_callbacks() noexcept {
    if (window_ == nullptr) {
        return;
    }
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);
}

bool Window::should_close() const noexcept {
    return window_ != nullptr && glfwWindowShouldClose(window_) == GLFW_TRUE;
}

void Window::poll_events() const noexcept { glfwPollEvents(); }

std::pair<int, int> Window::framebuffer_size() const noexcept {
    int width = 0;
    int height = 0;
    if (window_ != nullptr) {
        glfwGetFramebufferSize(window_, &width, &height);
    }
    return {width, height};
}

bool Window::was_resized() noexcept { return resized_; }

bool Window::key_pressed(int glfw_key) const noexcept {
    return window_ != nullptr && glfwGetKey(window_, glfw_key) == GLFW_PRESS;
}

void Window::framebuffer_resize_callback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->resized_ = true;
    }
}

}  // namespace kart::platform
