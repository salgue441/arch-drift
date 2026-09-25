/// @file Track.cpp
/// @brief Builds a flat oval road mesh and samples ground height.

#include "sim/Track.hpp"

#include <cmath>
#include <cstdint>

namespace kart::sim {
namespace {

[[nodiscard]] float horizontal_radius(const glm::vec3& p) noexcept {
    return std::sqrt(p.x * p.x + p.z * p.z);
}

}  // namespace

Track::Track() { build_mesh(); }

TrackSample Track::sample(const glm::vec3& position) const noexcept {
    TrackSample s{};
    s.height = 0.0F;
    s.normal = {0.0F, 1.0F, 0.0F};

    const float r = horizontal_radius(position);
    // Soft wall: push height cue is unused; walls handled later. Keep flat for Phase 1.
    (void)r;
    return s;
}

void Track::build_mesh() {
    mesh_ = {};

    // Grass disc (triangle fan from center).
    constexpr int kGrassSegments = 64;
    const glm::vec3 grass{0.18F, 0.42F, 0.16F};
    const std::uint32_t center = 0;
    mesh_.positions.push_back({0.0F, -0.02F, 0.0F});
    mesh_.normals.push_back({0.0F, 1.0F, 0.0F});
    mesh_.colors.push_back(grass);

    for (int i = 0; i <= kGrassSegments; ++i) {
        const float a = (static_cast<float>(i) / static_cast<float>(kGrassSegments)) *
                        (2.0F * 3.14159265F);
        mesh_.positions.push_back({std::cos(a) * (outer_radius_ + 18.0F), -0.02F,
                                   std::sin(a) * (outer_radius_ + 18.0F)});
        mesh_.normals.push_back({0.0F, 1.0F, 0.0F});
        mesh_.colors.push_back(grass);
    }
    for (int i = 1; i <= kGrassSegments; ++i) {
        mesh_.indices.push_back(center);
        mesh_.indices.push_back(static_cast<std::uint32_t>(i));
        mesh_.indices.push_back(static_cast<std::uint32_t>(i + 1));
    }

    // Asphalt ring between inner and outer radius.
    constexpr int kRoadSegments = 96;
    const glm::vec3 asphalt{0.22F, 0.22F, 0.24F};
    const glm::vec3 line{0.85F, 0.85F, 0.55F};
    const auto base = static_cast<std::uint32_t>(mesh_.positions.size());

    for (int i = 0; i <= kRoadSegments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kRoadSegments);
        const float a = t * (2.0F * 3.14159265F);
        const float c = std::cos(a);
        const float s = std::sin(a);
        const bool stripe = (i % 8) < 2;

        mesh_.positions.push_back({c * inner_radius_, 0.0F, s * inner_radius_});
        mesh_.normals.push_back({0.0F, 1.0F, 0.0F});
        mesh_.colors.push_back(stripe ? line : asphalt);

        mesh_.positions.push_back({c * outer_radius_, 0.0F, s * outer_radius_});
        mesh_.normals.push_back({0.0F, 1.0F, 0.0F});
        mesh_.colors.push_back(asphalt);
    }

    for (int i = 0; i < kRoadSegments; ++i) {
        const auto i0 = base + static_cast<std::uint32_t>(i * 2);
        const auto i1 = i0 + 1;
        const auto i2 = i0 + 2;
        const auto i3 = i0 + 3;
        mesh_.indices.push_back(i0);
        mesh_.indices.push_back(i2);
        mesh_.indices.push_back(i1);
        mesh_.indices.push_back(i1);
        mesh_.indices.push_back(i2);
        mesh_.indices.push_back(i3);
    }
}

}  // namespace kart::sim
