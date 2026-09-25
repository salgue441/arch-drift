#pragma once

#include "core/Result.hpp"
#include "platform/Window.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace kart::render {

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

    [[nodiscard]] VoidResult draw_frame();
    [[nodiscard]] VoidResult recreate_swapchain();

private:
    struct QueueFamilyIndices {
        uint32_t graphics = UINT32_MAX;
        uint32_t present = UINT32_MAX;

        [[nodiscard]] bool complete() const noexcept {
            return graphics != UINT32_MAX && present != UINT32_MAX;
        }
    };

    static constexpr int kMaxFramesInFlight = 2;

    [[nodiscard]] VoidResult create_instance();
    [[nodiscard]] VoidResult setup_debug_messenger();
    [[nodiscard]] VoidResult create_surface();
    [[nodiscard]] VoidResult pick_physical_device();
    [[nodiscard]] VoidResult create_device();
    [[nodiscard]] VoidResult create_swapchain();
    [[nodiscard]] VoidResult create_image_views();
    [[nodiscard]] VoidResult create_render_pass();
    [[nodiscard]] VoidResult create_graphics_pipeline();
    [[nodiscard]] VoidResult create_framebuffers();
    [[nodiscard]] VoidResult create_command_pool();
    [[nodiscard]] VoidResult create_command_buffers();
    [[nodiscard]] VoidResult create_sync_objects();

    void cleanup_swapchain() noexcept;
    void destroy() noexcept;

    [[nodiscard]] Result<VkShaderModule> load_shader_module(const std::filesystem::path& path) const;
    [[nodiscard]] QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;
    [[nodiscard]] bool is_device_suitable(VkPhysicalDevice device) const;

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

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;

    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_;

    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::size_t current_frame_ = 0;

    bool enable_validation_ = false;
};

}  // namespace kart::render
