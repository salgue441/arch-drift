#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

/// @file MeshData.hpp
/// @brief CPU-side interleaved mesh used before GPU upload.

namespace kart::render {

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 color{};
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

[[nodiscard]] MeshData make_box(const glm::vec3& half_extents, const glm::vec3& color);
[[nodiscard]] MeshData from_track(const std::vector<glm::vec3>& positions,
                                  const std::vector<glm::vec3>& normals,
                                  const std::vector<glm::vec3>& colors,
                                  const std::vector<std::uint32_t>& indices);

}  // namespace kart::render
