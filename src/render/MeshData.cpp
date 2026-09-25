/// @file MeshData.cpp
/// @brief CPU mesh builders for placeholder track and kart geometry.

#include "render/MeshData.hpp"

namespace kart::render {
namespace {

void add_face(MeshData& mesh, const glm::vec3& n, const glm::vec3& a, const glm::vec3& b,
              const glm::vec3& c, const glm::vec3& d, const glm::vec3& color) {
    const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({a, n, color});
    mesh.vertices.push_back({b, n, color});
    mesh.vertices.push_back({c, n, color});
    mesh.vertices.push_back({d, n, color});
    mesh.indices.push_back(base + 0);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 0);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
}

}  // namespace

MeshData make_box(const glm::vec3& half_extents, const glm::vec3& color) {
    MeshData mesh;
    const float x = half_extents.x;
    const float y = half_extents.y;
    const float z = half_extents.z;

    add_face(mesh, {0, 0, 1}, {-x, -y, z}, {x, -y, z}, {x, y, z}, {-x, y, z}, color);
    add_face(mesh, {0, 0, -1}, {x, -y, -z}, {-x, -y, -z}, {-x, y, -z}, {x, y, -z}, color);
    add_face(mesh, {0, 1, 0}, {-x, y, z}, {x, y, z}, {x, y, -z}, {-x, y, -z}, color);
    add_face(mesh, {0, -1, 0}, {-x, -y, -z}, {x, -y, -z}, {x, -y, z}, {-x, -y, z}, color);
    add_face(mesh, {1, 0, 0}, {x, -y, z}, {x, -y, -z}, {x, y, -z}, {x, y, z}, color);
    add_face(mesh, {-1, 0, 0}, {-x, -y, -z}, {-x, -y, z}, {-x, y, z}, {-x, y, -z}, color);
    return mesh;
}

MeshData from_track(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals,
                    const std::vector<glm::vec3>& colors, const std::vector<std::uint32_t>& indices) {
    MeshData mesh;
    mesh.vertices.resize(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
        mesh.vertices[i] = {positions[i], normals[i], colors[i]};
    }
    mesh.indices = indices;
    return mesh;
}

}  // namespace kart::render
