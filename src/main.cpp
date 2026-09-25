#include "core/Result.hpp"
#include "platform/Window.hpp"
#include "render/Renderer.hpp"

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

[[nodiscard]] std::filesystem::path resolve_shader_dir() {
    const std::filesystem::path candidates[] = {
        std::filesystem::path(KART_SHADER_DIR),
        std::filesystem::current_path() / "shaders",
        std::filesystem::current_path() / "build" / "shaders",
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "triangle.vert.spv") &&
            std::filesystem::exists(candidate / "triangle.frag.spv")) {
            return candidate;
        }
    }
    return std::filesystem::path(KART_SHADER_DIR);
}

}  // namespace

int main() {
    using kart::Error;
    using kart::to_string;

    auto window = kart::platform::Window::create({.width = 1280, .height = 720, .title = "Qantara Kart"});
    if (!window) {
        std::cerr << "Failed to create window: " << to_string(window.error()) << '\n';
        return EXIT_FAILURE;
    }
    window->attach_callbacks();

    auto renderer = kart::render::Renderer::create(*window, resolve_shader_dir());
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << to_string(renderer.error()) << '\n';
        return EXIT_FAILURE;
    }

    while (!window->should_close()) {
        window->poll_events();
        if (window->key_pressed(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window->handle(), GLFW_TRUE);
        }

        if (auto frame = renderer->draw_frame(); !frame) {
            std::cerr << "Frame failed: " << to_string(frame.error()) << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
