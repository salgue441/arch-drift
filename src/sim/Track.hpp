#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

/// @file Track.hpp
/// @brief Flat oval track with height/normal queries and CPU mesh data.

namespace kart::sim {

struct TrackSample {
    float height = 0.0F;
    glm::vec3 normal{0.0F, 1.0F, 0.0F};
};

struct TrackMeshData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> colors;
    std::vector<std::uint32_t> indices;
};

/// Procedural asphalt oval on a grass plane (Phase 1 placeholder).
class Track {
public:
    Track();

    [[nodiscard]] TrackSample sample(const glm::vec3& position) const noexcept;
    [[nodiscard]] const TrackMeshData& mesh() const noexcept { return mesh_; }
    [[nodiscard]] glm::vec3 start_position() const noexcept { return {0.0F, 0.0F, 35.0F}; }
    [[nodiscard]] float start_yaw() const noexcept { return 0.0F; }

private:
    void build_mesh();

    float outer_radius_ = 48.0F;
    float inner_radius_ = 28.0F;
    TrackMeshData mesh_;
};

}  // namespace kart::sim
