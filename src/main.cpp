/// @file main.cpp
/// @brief Archdrift entry (Phase 1: track, kart, chase camera, keyboard drive).

#include "core/Result.hpp"
#include "input/KeyboardSource.hpp"
#include "platform/Window.hpp"
#include "render/MeshData.hpp"
#include "render/Renderer.hpp"
#include "sim/Camera.hpp"
#include "sim/Kart.hpp"
#include "sim/Track.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <chrono>
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
        if (std::filesystem::exists(candidate / "mesh.vert.spv") &&
            std::filesystem::exists(candidate / "mesh.frag.spv")) {
            return candidate;
        }
    }
    return std::filesystem::path(KART_SHADER_DIR);
}

}  // namespace

int main() {
    using kart::to_string;
    using clock = std::chrono::steady_clock;

    auto window = kart::platform::Window::create({.width = 1280, .height = 720, .title = "Archdrift"});
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

    kart::sim::Track track;
    kart::sim::KartState kart_state{};
    kart_state.position = track.start_position();
    kart_state.yaw = track.start_yaw();
    kart::sim::Kart player(kart_state);
    kart::sim::Camera camera{};
    kart::input::KeyboardSource keyboard(*window);

    const auto track_cpu = kart::render::from_track(track.mesh().positions, track.mesh().normals,
                                                    track.mesh().colors, track.mesh().indices);
    auto track_gpu = renderer->create_mesh(track_cpu);
    if (!track_gpu) {
        std::cerr << "Failed to upload track mesh: " << to_string(track_gpu.error()) << '\n';
        return EXIT_FAILURE;
    }
    auto kart_gpu = renderer->create_mesh(kart::render::make_box({0.6F, 0.35F, 1.0F}, player.state().color));
    if (!kart_gpu) {
        std::cerr << "Failed to upload kart mesh: " << to_string(kart_gpu.error()) << '\n';
        return EXIT_FAILURE;
    }

    constexpr float kFixedDt = 1.0F / 60.0F;
    float accumulator = 0.0F;
    auto previous = clock::now();

    while (!window->should_close()) {
        window->poll_events();
        if (window->key_pressed(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window->handle(), GLFW_TRUE);
        }

        const auto now = clock::now();
        const float frame_dt =
            std::chrono::duration<float>(now - previous).count();
        previous = now;
        accumulator += std::min(frame_dt, 0.05F);

        const auto input = keyboard.sample();
        while (accumulator >= kFixedDt) {
            const auto ground = track.sample(player.state().position);
            player.tick(kFixedDt, input, ground.height, ground.normal);
            accumulator -= kFixedDt;
        }

        const glm::mat4 view = camera.view_matrix(player.state());
        const glm::mat4 proj = camera.proj_matrix(renderer->aspect_ratio());
        const glm::mat4 view_proj = proj * view;

        glm::mat4 kart_model = glm::translate(glm::mat4{1.0F}, player.state().position);
        kart_model = glm::rotate(kart_model, player.state().yaw, glm::vec3{0.0F, 1.0F, 0.0F});
        kart_model = glm::translate(kart_model, glm::vec3{0.0F, 0.35F, 0.0F});

        const std::array draws = {
            kart::render::DrawItem{.mesh = &*track_gpu, .model = glm::mat4{1.0F}},
            kart::render::DrawItem{.mesh = &*kart_gpu, .model = kart_model},
        };

        if (auto frame = renderer->draw_frame({.view_proj = view_proj, .draws = draws}); !frame) {
            std::cerr << "Frame failed: " << to_string(frame.error()) << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
