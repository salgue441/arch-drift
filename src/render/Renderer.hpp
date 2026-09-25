#pragma once

#include "core/Result.hpp"
#include "platform/Window.hpp"
#include "render/MeshData.hpp"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

/// @file Renderer.hpp
/// @brief Vulkan renderer with depth-tested mesh draws (Phase 1).

namespace kart::render {

/// GPU mesh owned by the renderer (vertex + index buffers).
class GpuMesh {
public:
    GpuMesh() = default;
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;
    ~GpuMesh();

    [[nodiscard]] VkBuffer vertex_buffer() const noexcept { return vertex_buffer_; }
    [[nodiscard]] VkBuffer index_buffer() const noexcept { return index_buffer_; }
    [[nodiscard]] std::uint32_t index_count() const noexcept { return index_count_; }

private:
    friend class Renderer;
    VkDevice device_ = VK_NULL_HANDLE;
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_memory_ = VK_NULL_HANDLE;
    VkBuffer index_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory index_memory_ = VK_NULL_HANDLE;
    std::uint32_t index_count_ = 0;
};

struct DrawItem {
    const GpuMesh* mesh = nullptr;
    glm::mat4 model{1.0F};
    glm::vec4 color_mul{1.0F, 1.0F, 1.0F, 1.0F};
};

struct FrameScene {
    glm::mat4 view_proj{1.0F};
    std::span<const DrawItem> draws;
};

/// Owns Vulkan device resources and presents mesh scenes.
/// @thread_safety Main thread only.
/// @warning Window must outlive the renderer.
class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept;
    Renderer& operator=(Renderer&&) noexcept;
    ~Renderer();

    [[nodiscard]] static Result<Renderer> create(platform::Window& window,
                                                 const std::filesystem::path& shader_dir);

    [[nodiscard]] Result<GpuMesh> create_mesh(const MeshData& data);
    [[nodiscard]] VoidResult draw_frame(const FrameScene& scene);
    [[nodiscard]] VoidResult recreate_swapchain();
    [[nodiscard]] float aspect_ratio() const noexcept;

private:
    struct QueueFamilyIndices {
        uint32_t graphics = UINT32_MAX;
        uint32_t present = UINT32_MAX;
        [[nodiscard]] bool complete() const noexcept {
            return graphics != UINT32_MAX && present != UINT32_MAX;
        }
    };

    struct ObjectUBO {
        glm::mat4 mvp{1.0F};
        glm::mat4 model{1.0F};
        glm::vec4 color_mul{1.0F};
    };

    static constexpr int kMaxFramesInFlight = 2;
    static constexpr int kMaxDrawsPerFrame = 16;
    static constexpr VkDeviceSize kObjectUboAlignedSize = 256;  // minUniformBufferOffsetAlignment-safe


    [[nodiscard]] VoidResult create_instance();
    [[nodiscard]] VoidResult setup_debug_messenger();
    [[nodiscard]] VoidResult create_surface();
    [[nodiscard]] VoidResult pick_physical_device();
    [[nodiscard]] VoidResult create_device();
    [[nodiscard]] VoidResult create_swapchain();
    [[nodiscard]] VoidResult create_image_views();
    [[nodiscard]] VoidResult create_depth_resources();
    [[nodiscard]] VoidResult create_render_pass();
    [[nodiscard]] VoidResult create_descriptor_set_layout();
    [[nodiscard]] VoidResult create_graphics_pipeline();
    [[nodiscard]] VoidResult create_framebuffers();
    [[nodiscard]] VoidResult create_command_pool();
    [[nodiscard]] VoidResult create_uniform_buffers();
    [[nodiscard]] VoidResult create_descriptor_pool();
    [[nodiscard]] VoidResult create_descriptor_sets();
    [[nodiscard]] VoidResult create_command_buffers();
    [[nodiscard]] VoidResult create_sync_objects();

    void cleanup_swapchain() noexcept;
    void destroy() noexcept;

    [[nodiscard]] Result<VkShaderModule> load_shader_module(const std::filesystem::path& path) const;
    [[nodiscard]] QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;
    [[nodiscard]] bool is_device_suitable(VkPhysicalDevice device) const;
    [[nodiscard]] uint32_t find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags props) const;
    [[nodiscard]] VoidResult create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags props, VkBuffer& buffer,
                                           VkDeviceMemory& memory);
    [[nodiscard]] VoidResult upload_buffer(VkBuffer dst, const void* data, VkDeviceSize size);

    platform::Window* window_ = nullptr;
    std::filesystem::path shader_dir_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    QueueFamilyIndices queue_families_{};

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchain_format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchain_extent_{};
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    std::vector<VkFramebuffer> swapchain_framebuffers_;

    VkImage depth_image_ = VK_NULL_HANDLE;
    VkDeviceMemory depth_memory_ = VK_NULL_HANDLE;
    VkImageView depth_view_ = VK_NULL_HANDLE;
    VkFormat depth_format_ = VK_FORMAT_D32_SFLOAT;

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;

    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_;

    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;
    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_memories_;
    std::vector<void*> uniform_mapped_;

    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::size_t current_frame_ = 0;

    bool enable_validation_ = false;
};

}  // namespace kart::render
