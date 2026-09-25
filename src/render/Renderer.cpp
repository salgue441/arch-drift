/// @file Renderer.cpp
/// @brief Vulkan mesh renderer: device setup, depth swapchain, and scene draws.

#include "render/Renderer.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>

namespace kart::render {
namespace {

constexpr std::array kValidationLayers = {"VK_LAYER_KHRONOS_validation"};
constexpr std::array kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

[[nodiscard]] bool check_validation_layer_support() {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> available(count);
    vkEnumerateInstanceLayerProperties(&count, available.data());
    for (const char* layer : kValidationLayers) {
        const bool found =
            std::any_of(available.begin(), available.end(), [&](const VkLayerProperties& props) {
                return std::strcmp(props.layerName, layer) == 0;
            });
        if (!found) {
            return false;
        }
    }
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                              VkDebugUtilsMessageTypeFlagsEXT,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data,
                                              void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[archdrift][vulkan] " << data->pMessage << '\n';
    }
    return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance,
                                          const VkDebugUtilsMessengerCreateInfoEXT* info,
                                          const VkAllocationCallbacks* allocator,
                                          VkDebugUtilsMessengerEXT* messenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    return func ? func(instance, info, allocator, messenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                       const VkAllocationCallbacks* allocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func) {
        func(instance, messenger, allocator);
    }
}

[[nodiscard]] std::vector<char> read_file_bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        return {};
    }
    const auto size = static_cast<std::size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(size));
    return buffer;
}

}  // namespace

GpuMesh::GpuMesh(GpuMesh&& other) noexcept {
    *this = std::move(other);
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    this->~GpuMesh();
    device_ = other.device_;
    vertex_buffer_ = std::exchange(other.vertex_buffer_, VK_NULL_HANDLE);
    vertex_memory_ = std::exchange(other.vertex_memory_, VK_NULL_HANDLE);
    index_buffer_ = std::exchange(other.index_buffer_, VK_NULL_HANDLE);
    index_memory_ = std::exchange(other.index_memory_, VK_NULL_HANDLE);
    index_count_ = other.index_count_;
    other.device_ = VK_NULL_HANDLE;
    other.index_count_ = 0;
    return *this;
}

GpuMesh::~GpuMesh() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    if (index_buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, index_buffer_, nullptr);
    }
    if (index_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, index_memory_, nullptr);
    }
    if (vertex_buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, vertex_buffer_, nullptr);
    }
    if (vertex_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vertex_memory_, nullptr);
    }
}

Renderer::Renderer(Renderer&& other) noexcept { *this = std::move(other); }

Renderer& Renderer::operator=(Renderer&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    destroy();
    window_ = other.window_;
    shader_dir_ = std::move(other.shader_dir_);
    instance_ = std::exchange(other.instance_, VK_NULL_HANDLE);
    debug_messenger_ = std::exchange(other.debug_messenger_, VK_NULL_HANDLE);
    surface_ = std::exchange(other.surface_, VK_NULL_HANDLE);
    physical_device_ = std::exchange(other.physical_device_, VK_NULL_HANDLE);
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    graphics_queue_ = other.graphics_queue_;
    present_queue_ = other.present_queue_;
    queue_families_ = other.queue_families_;
    swapchain_ = std::exchange(other.swapchain_, VK_NULL_HANDLE);
    swapchain_format_ = other.swapchain_format_;
    swapchain_extent_ = other.swapchain_extent_;
    swapchain_images_ = std::move(other.swapchain_images_);
    swapchain_image_views_ = std::move(other.swapchain_image_views_);
    swapchain_framebuffers_ = std::move(other.swapchain_framebuffers_);
    depth_image_ = std::exchange(other.depth_image_, VK_NULL_HANDLE);
    depth_memory_ = std::exchange(other.depth_memory_, VK_NULL_HANDLE);
    depth_view_ = std::exchange(other.depth_view_, VK_NULL_HANDLE);
    depth_format_ = other.depth_format_;
    render_pass_ = std::exchange(other.render_pass_, VK_NULL_HANDLE);
    descriptor_set_layout_ = std::exchange(other.descriptor_set_layout_, VK_NULL_HANDLE);
    pipeline_layout_ = std::exchange(other.pipeline_layout_, VK_NULL_HANDLE);
    graphics_pipeline_ = std::exchange(other.graphics_pipeline_, VK_NULL_HANDLE);
    command_pool_ = std::exchange(other.command_pool_, VK_NULL_HANDLE);
    command_buffers_ = std::move(other.command_buffers_);
    descriptor_pool_ = std::exchange(other.descriptor_pool_, VK_NULL_HANDLE);
    descriptor_sets_ = std::move(other.descriptor_sets_);
    uniform_buffers_ = std::move(other.uniform_buffers_);
    uniform_memories_ = std::move(other.uniform_memories_);
    uniform_mapped_ = std::move(other.uniform_mapped_);
    image_available_semaphores_ = std::move(other.image_available_semaphores_);
    render_finished_semaphores_ = std::move(other.render_finished_semaphores_);
    in_flight_fences_ = std::move(other.in_flight_fences_);
    current_frame_ = other.current_frame_;
    enable_validation_ = other.enable_validation_;
    other.window_ = nullptr;
    return *this;
}

Renderer::~Renderer() { destroy(); }

Result<Renderer> Renderer::create(platform::Window& window, const std::filesystem::path& shader_dir) {
    Renderer renderer;
    renderer.window_ = &window;
    renderer.shader_dir_ = shader_dir;

#ifndef NDEBUG
    renderer.enable_validation_ = check_validation_layer_support();
    if (!renderer.enable_validation_) {
        std::cerr << "[archdrift] validation layers unavailable; continuing\n";
    }
#endif

    const auto steps = std::array<VoidResult (Renderer::*)(), 15>{
        &Renderer::create_instance,
        &Renderer::setup_debug_messenger,
        &Renderer::create_surface,
        &Renderer::pick_physical_device,
        &Renderer::create_device,
        &Renderer::create_swapchain,
        &Renderer::create_image_views,
        &Renderer::create_depth_resources,
        &Renderer::create_render_pass,
        &Renderer::create_descriptor_set_layout,
        &Renderer::create_graphics_pipeline,
        &Renderer::create_framebuffers,
        &Renderer::create_command_pool,
        &Renderer::create_uniform_buffers,
        &Renderer::create_descriptor_pool,
    };
    // create_descriptor_sets, command buffers, sync separately after pool
    for (auto step : steps) {
        if (auto r = (renderer.*step)(); !r) {
            return Unexpected<Renderer>(r.error());
        }
    }
    if (auto r = renderer.create_descriptor_sets(); !r) {
        return Unexpected<Renderer>(r.error());
    }
    if (auto r = renderer.create_command_buffers(); !r) {
        return Unexpected<Renderer>(r.error());
    }
    if (auto r = renderer.create_sync_objects(); !r) {
        return Unexpected<Renderer>(r.error());
    }
    return renderer;
}

float Renderer::aspect_ratio() const noexcept {
    if (swapchain_extent_.height == 0) {
        return 1.0F;
    }
    return static_cast<float>(swapchain_extent_.width) /
           static_cast<float>(swapchain_extent_.height);
}

uint32_t Renderer::find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties mem{};
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem);
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i) {
        if ((type_bits & (1u << i)) != 0u &&
            (mem.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return UINT32_MAX;
}

VoidResult Renderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags props, VkBuffer& buffer,
                                   VkDeviceMemory& memory) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &info, nullptr, &buffer) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device_, buffer, &req);
    const uint32_t type = find_memory_type(req.memoryTypeBits, props);
    if (type == UINT32_MAX) {
        return std::unexpected(Error::DeviceCreationFailed);
    }

    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = type;
    if (vkAllocateMemory(device_, &alloc, nullptr, &memory) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }
    vkBindBufferMemory(device_, buffer, memory, 0);
    return Ok();
}

VoidResult Renderer::upload_buffer(VkBuffer dst, const void* data, VkDeviceSize size) {
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem = VK_NULL_HANDLE;
    if (auto r = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               staging, staging_mem);
        !r) {
        return r;
    }
    void* mapped = nullptr;
    vkMapMemory(device_, staging_mem, 0, size, 0, &mapped);
    std::memcpy(mapped, data, static_cast<std::size_t>(size));
    vkUnmapMemory(device_, staging_mem);

    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandPool = command_pool_;
    alloc.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &alloc, &cmd);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy copy{0, 0, size};
    vkCmdCopyBuffer(cmd, staging, dst, 1, &copy);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);
    vkFreeCommandBuffers(device_, command_pool_, 1, &cmd);
    vkDestroyBuffer(device_, staging, nullptr);
    vkFreeMemory(device_, staging_mem, nullptr);
    return Ok();
}

Result<GpuMesh> Renderer::create_mesh(const MeshData& data) {
    if (data.vertices.empty() || data.indices.empty()) {
        return Unexpected<GpuMesh>(Error::Unknown);
    }
    GpuMesh mesh;
    mesh.device_ = device_;
    mesh.index_count_ = static_cast<std::uint32_t>(data.indices.size());

    const VkDeviceSize vsize = sizeof(Vertex) * data.vertices.size();
    const VkDeviceSize isize = sizeof(std::uint32_t) * data.indices.size();

    if (auto r = create_buffer(vsize,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertex_buffer_,
                               mesh.vertex_memory_);
        !r) {
        return Unexpected<GpuMesh>(r.error());
    }
    if (auto r = create_buffer(isize,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.index_buffer_,
                               mesh.index_memory_);
        !r) {
        return Unexpected<GpuMesh>(r.error());
    }
    if (auto r = upload_buffer(mesh.vertex_buffer_, data.vertices.data(), vsize); !r) {
        return Unexpected<GpuMesh>(r.error());
    }
    if (auto r = upload_buffer(mesh.index_buffer_, data.indices.data(), isize); !r) {
        return Unexpected<GpuMesh>(r.error());
    }
    return mesh;
}

VoidResult Renderer::create_instance() {
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Archdrift";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName = "Archdrift";
    app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfw_count = 0;
    const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_count);
    if (!glfw_ext) {
        return std::unexpected(Error::VulkanUnavailable);
    }
    std::vector<const char*> extensions(glfw_ext, glfw_ext + glfw_count);
    if (enable_validation_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug{};
    if (enable_validation_) {
        info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        info.ppEnabledLayerNames = kValidationLayers.data();
        debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pfnUserCallback = debug_callback;
        info.pNext = &debug;
    }

    if (vkCreateInstance(&info, nullptr, &instance_) != VK_SUCCESS) {
        return std::unexpected(Error::VulkanUnavailable);
    }
    return Ok();
}

VoidResult Renderer::setup_debug_messenger() {
    if (!enable_validation_) {
        return Ok();
    }
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debug_callback;
    if (create_debug_utils_messenger_ext(instance_, &info, nullptr, &debug_messenger_) != VK_SUCCESS) {
        debug_messenger_ = VK_NULL_HANDLE;
    }
    return Ok();
}

VoidResult Renderer::create_surface() {
    if (glfwCreateWindowSurface(instance_, window_->handle(), nullptr, &surface_) != VK_SUCCESS) {
        return std::unexpected(Error::WindowInitFailed);
    }
    return Ok();
}

Renderer::QueueFamilyIndices Renderer::find_queue_families(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    for (uint32_t i = 0; i < count; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
            indices.graphics = i;
        }
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present);
        if (present) {
            indices.present = i;
        }
        if (indices.complete()) {
            break;
        }
    }
    return indices;
}

bool Renderer::is_device_suitable(VkPhysicalDevice device) const {
    if (!find_queue_families(device).complete()) {
        return false;
    }
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> available(ext_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, available.data());
    std::set<std::string> required(kDeviceExtensions.begin(), kDeviceExtensions.end());
    for (const auto& e : available) {
        required.erase(e.extensionName);
    }
    if (!required.empty()) {
        return false;
    }
    uint32_t formats = 0;
    uint32_t modes = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formats, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &modes, nullptr);
    return formats > 0 && modes > 0;
}

VoidResult Renderer::pick_physical_device() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        return std::unexpected(Error::NoSuitableGpu);
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    for (auto d : devices) {
        if (is_device_suitable(d)) {
            physical_device_ = d;
            queue_families_ = find_queue_families(d);
            return Ok();
        }
    }
    return std::unexpected(Error::NoSuitableGpu);
}

VoidResult Renderer::create_device() {
    std::set<uint32_t> unique{queue_families_.graphics, queue_families_.present};
    std::vector<VkDeviceQueueCreateInfo> queues;
    const float priority = 1.0F;
    for (uint32_t family : unique) {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = family;
        q.queueCount = 1;
        q.pQueuePriorities = &priority;
        queues.push_back(q);
    }
    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    info.pQueueCreateInfos = queues.data();
    info.pEnabledFeatures = &features;
    info.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
    info.ppEnabledExtensionNames = kDeviceExtensions.data();
    if (enable_validation_) {
        info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        info.ppEnabledLayerNames = kValidationLayers.data();
    }
    if (vkCreateDevice(physical_device_, &info, nullptr, &device_) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }
    vkGetDeviceQueue(device_, queue_families_.graphics, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, queue_families_.present, 0, &present_queue_);
    return Ok();
}

VoidResult Renderer::create_swapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &caps);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());

    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &mode_count, nullptr);
    std::vector<VkPresentModeKHR> modes(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &mode_count, modes.data());

    VkSurfaceFormatKHR surface_format = formats.front();
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = f;
            break;
        }
    }
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = m;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (caps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        const auto [w, h] = window_->framebuffer_size();
        extent.width = std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width,
                                  caps.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height,
                                   caps.maxImageExtent.height);
    }
    if (extent.width == 0 || extent.height == 0) {
        return std::unexpected(Error::SwapchainFailed);
    }

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface_;
    info.minImageCount = image_count;
    info.imageFormat = surface_format.format;
    info.imageColorSpace = surface_format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    const uint32_t qidx[] = {queue_families_.graphics, queue_families_.present};
    if (queue_families_.graphics != queue_families_.present) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = qidx;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device_, &info, nullptr, &swapchain_) != VK_SUCCESS) {
        return std::unexpected(Error::SwapchainFailed);
    }
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());
    swapchain_format_ = surface_format.format;
    swapchain_extent_ = extent;
    return Ok();
}

VoidResult Renderer::create_image_views() {
    swapchain_image_views_.resize(swapchain_images_.size());
    for (std::size_t i = 0; i < swapchain_images_.size(); ++i) {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = swapchain_images_[i];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = swapchain_format_;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device_, &info, nullptr, &swapchain_image_views_[i]) != VK_SUCCESS) {
            return std::unexpected(Error::SwapchainFailed);
        }
    }
    return Ok();
}

VoidResult Renderer::create_depth_resources() {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent = {swapchain_extent_.width, swapchain_extent_.height, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.format = depth_format_;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device_, &info, nullptr, &depth_image_) != VK_SUCCESS) {
        return std::unexpected(Error::SwapchainFailed);
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device_, depth_image_, &req);
    const uint32_t type =
        find_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (type == UINT32_MAX) {
        return std::unexpected(Error::SwapchainFailed);
    }
    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = type;
    if (vkAllocateMemory(device_, &alloc, nullptr, &depth_memory_) != VK_SUCCESS) {
        return std::unexpected(Error::SwapchainFailed);
    }
    vkBindImageMemory(device_, depth_image_, depth_memory_, 0);

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = depth_image_;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = depth_format_;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &view, nullptr, &depth_view_) != VK_SUCCESS) {
        return std::unexpected(Error::SwapchainFailed);
    }
    return Ok();
}

VoidResult Renderer::create_render_pass() {
    VkAttachmentDescription color{};
    color.format = swapchain_format_;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = depth_format_;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_ref{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const std::array attachments = {color, depth};
    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dep;
    if (vkCreateRenderPass(device_, &info, nullptr, &render_pass_) != VK_SUCCESS) {
        return std::unexpected(Error::PipelineFailed);
    }
    return Ok();
}

VoidResult Renderer::create_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(device_, &info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        return std::unexpected(Error::PipelineFailed);
    }
    return Ok();
}

Result<VkShaderModule> Renderer::load_shader_module(const std::filesystem::path& path) const {
    const auto code = read_file_bytes(path);
    if (code.empty()) {
        return Unexpected<VkShaderModule>(Error::ShaderLoadFailed);
    }
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_, &info, nullptr, &module) != VK_SUCCESS) {
        return Unexpected<VkShaderModule>(Error::ShaderLoadFailed);
    }
    return module;
}

VoidResult Renderer::create_graphics_pipeline() {
    auto vert = load_shader_module(shader_dir_ / "mesh.vert.spv");
    auto frag = load_shader_module(shader_dir_ / "mesh.frag.spv");
    if (!vert) {
        return std::unexpected(vert.error());
    }
    if (!frag) {
        vkDestroyShaderModule(device_, *vert, nullptr);
        return std::unexpected(frag.error());
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = *vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = *frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attrs{};
    attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
    attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
    attrs[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)};

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertex_input.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blend_attach{};
    blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attach;

    const std::array dyn = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<uint32_t>(dyn.size());
    dynamic.pDynamicStates = dyn.data();

    VkPipelineLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &descriptor_set_layout_;
    if (vkCreatePipelineLayout(device_, &layout, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, *vert, nullptr);
        vkDestroyShaderModule(device_, *frag, nullptr);
        return std::unexpected(Error::PipelineFailed);
    }

    VkGraphicsPipelineCreateInfo pipe{};
    pipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe.stageCount = 2;
    pipe.pStages = stages;
    pipe.pVertexInputState = &vertex_input;
    pipe.pInputAssemblyState = &input_assembly;
    pipe.pViewportState = &viewport;
    pipe.pRasterizationState = &raster;
    pipe.pMultisampleState = &ms;
    pipe.pDepthStencilState = &depth;
    pipe.pColorBlendState = &blend;
    pipe.pDynamicState = &dynamic;
    pipe.layout = pipeline_layout_;
    pipe.renderPass = render_pass_;
    pipe.subpass = 0;

    const VkResult pr =
        vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipe, nullptr, &graphics_pipeline_);
    vkDestroyShaderModule(device_, *vert, nullptr);
    vkDestroyShaderModule(device_, *frag, nullptr);
    if (pr != VK_SUCCESS) {
        return std::unexpected(Error::PipelineFailed);
    }
    return Ok();
}

VoidResult Renderer::create_framebuffers() {
    swapchain_framebuffers_.resize(swapchain_image_views_.size());
    for (std::size_t i = 0; i < swapchain_image_views_.size(); ++i) {
        const std::array attachments = {swapchain_image_views_[i], depth_view_};
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = render_pass_;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.width = swapchain_extent_.width;
        info.height = swapchain_extent_.height;
        info.layers = 1;
        if (vkCreateFramebuffer(device_, &info, nullptr, &swapchain_framebuffers_[i]) != VK_SUCCESS) {
            return std::unexpected(Error::SwapchainFailed);
        }
    }
    return Ok();
}

VoidResult Renderer::create_command_pool() {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = queue_families_.graphics;
    if (vkCreateCommandPool(device_, &info, nullptr, &command_pool_) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }
    return Ok();
}

VoidResult Renderer::create_uniform_buffers() {
    uniform_buffers_.resize(kMaxFramesInFlight);
    uniform_memories_.resize(kMaxFramesInFlight);
    uniform_mapped_.resize(kMaxFramesInFlight);
    const VkDeviceSize buffer_size = kObjectUboAlignedSize * static_cast<VkDeviceSize>(kMaxDrawsPerFrame);
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        if (auto r = create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   uniform_buffers_[static_cast<std::size_t>(i)],
                                   uniform_memories_[static_cast<std::size_t>(i)]);
            !r) {
            return r;
        }
        vkMapMemory(device_, uniform_memories_[static_cast<std::size_t>(i)], 0, buffer_size, 0,
                    &uniform_mapped_[static_cast<std::size_t>(i)]);
    }
    return Ok();
}

VoidResult Renderer::create_descriptor_pool() {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_size.descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &pool_size;
    info.maxSets = static_cast<uint32_t>(kMaxFramesInFlight);
    if (vkCreateDescriptorPool(device_, &info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }
    return Ok();
}

VoidResult Renderer::create_descriptor_sets() {
    std::vector<VkDescriptorSetLayout> layouts(kMaxFramesInFlight, descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc.descriptorPool = descriptor_pool_;
    alloc.descriptorSetCount = static_cast<uint32_t>(kMaxFramesInFlight);
    alloc.pSetLayouts = layouts.data();
    descriptor_sets_.resize(kMaxFramesInFlight);
    if (vkAllocateDescriptorSets(device_, &alloc, descriptor_sets_.data()) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        VkDescriptorBufferInfo buffer{};
        buffer.buffer = uniform_buffers_[static_cast<std::size_t>(i)];
        buffer.offset = 0;
        buffer.range = sizeof(ObjectUBO);
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptor_sets_[static_cast<std::size_t>(i)];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer;
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
    return Ok();
}

VoidResult Renderer::create_command_buffers() {
    command_buffers_.resize(kMaxFramesInFlight);
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = command_pool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());
    if (vkAllocateCommandBuffers(device_, &alloc, command_buffers_.data()) != VK_SUCCESS) {
        return std::unexpected(Error::DeviceCreationFailed);
    }
    return Ok();
}

VoidResult Renderer::create_sync_objects() {
    image_available_semaphores_.resize(kMaxFramesInFlight);
    render_finished_semaphores_.resize(kMaxFramesInFlight);
    in_flight_fences_.resize(kMaxFramesInFlight);
    VkSemaphoreCreateInfo sem{};
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateSemaphore(device_, &sem, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &sem, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fence, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
            return std::unexpected(Error::DeviceCreationFailed);
        }
    }
    return Ok();
}

void Renderer::cleanup_swapchain() noexcept {
    if (depth_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, depth_view_, nullptr);
        depth_view_ = VK_NULL_HANDLE;
    }
    if (depth_image_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, depth_image_, nullptr);
        depth_image_ = VK_NULL_HANDLE;
    }
    if (depth_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, depth_memory_, nullptr);
        depth_memory_ = VK_NULL_HANDLE;
    }
    for (auto fb : swapchain_framebuffers_) {
        vkDestroyFramebuffer(device_, fb, nullptr);
    }
    swapchain_framebuffers_.clear();
    for (auto view : swapchain_image_views_) {
        vkDestroyImageView(device_, view, nullptr);
    }
    swapchain_image_views_.clear();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

VoidResult Renderer::recreate_swapchain() {
    auto [w, h] = window_->framebuffer_size();
    while ((w == 0 || h == 0) && !window_->should_close()) {
        window_->poll_events();
        std::tie(w, h) = window_->framebuffer_size();
    }
    if (window_->should_close()) {
        return Ok();
    }
    vkDeviceWaitIdle(device_);
    cleanup_swapchain();
    if (auto r = create_swapchain(); !r) {
        return r;
    }
    if (auto r = create_image_views(); !r) {
        return r;
    }
    if (auto r = create_depth_resources(); !r) {
        return r;
    }
    if (auto r = create_framebuffers(); !r) {
        return r;
    }
    return Ok();
}

VoidResult Renderer::draw_frame(const FrameScene& scene) {
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult acquire =
        vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                              image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreate_swapchain();
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        return std::unexpected(Error::SwapchainFailed);
    }

    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
    vkResetCommandBuffer(command_buffers_[current_frame_], 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffers_[current_frame_], &begin);

    std::array<VkClearValue, 2> clears{};
    clears[0].color = {{0.45F, 0.72F, 0.95F, 1.0F}};
    clears[1].depthStencil = {1.0F, 0};

    VkRenderPassBeginInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass = render_pass_;
    rp.framebuffer = swapchain_framebuffers_[image_index];
    rp.renderArea.extent = swapchain_extent_;
    rp.clearValueCount = static_cast<uint32_t>(clears.size());
    rp.pClearValues = clears.data();

    vkCmdBeginRenderPass(command_buffers_[current_frame_], &rp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffers_[current_frame_], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline_);

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchain_extent_.width);
    viewport.height = static_cast<float>(swapchain_extent_.height);
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(command_buffers_[current_frame_], 0, 1, &viewport);
    VkRect2D scissor{};
    scissor.extent = swapchain_extent_;
    vkCmdSetScissor(command_buffers_[current_frame_], 0, 1, &scissor);

    // One UBO slot per draw with dynamic offsets (Phase 1 draw counts are tiny).
    int draw_index = 0;
    for (const DrawItem& item : scene.draws) {
        if (item.mesh == nullptr || draw_index >= kMaxDrawsPerFrame) {
            continue;
        }
        ObjectUBO ubo{};
        ubo.model = item.model;
        ubo.mvp = scene.view_proj * item.model;
        ubo.color_mul = item.color_mul;
        auto* base = static_cast<std::byte*>(uniform_mapped_[current_frame_]);
        std::memcpy(base + (static_cast<std::size_t>(draw_index) * kObjectUboAlignedSize), &ubo,
                    sizeof(ubo));

        const uint32_t dynamic_offset =
            static_cast<uint32_t>(draw_index) * static_cast<uint32_t>(kObjectUboAlignedSize);
        vkCmdBindDescriptorSets(command_buffers_[current_frame_], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_layout_, 0, 1, &descriptor_sets_[current_frame_], 1,
                                &dynamic_offset);
        VkBuffer vbo = item.mesh->vertex_buffer();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(command_buffers_[current_frame_], 0, 1, &vbo, &offset);
        vkCmdBindIndexBuffer(command_buffers_[current_frame_], item.mesh->index_buffer(), 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers_[current_frame_], item.mesh->index_count(), 1, 0, 0, 0);
        ++draw_index;
    }

    vkCmdEndRenderPass(command_buffers_[current_frame_]);
    vkEndCommandBuffer(command_buffers_[current_frame_]);

    const VkSemaphore wait_sem[] = {image_available_semaphores_[current_frame_]};
    const VkPipelineStageFlags wait_stage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSemaphore signal_sem[] = {render_finished_semaphores_[current_frame_]};

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = wait_sem;
    submit.pWaitDstStageMask = wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffers_[current_frame_];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = signal_sem;
    if (vkQueueSubmit(graphics_queue_, 1, &submit, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
        return std::unexpected(Error::Unknown);
    }

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = signal_sem;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &image_index;
    const VkResult pr = vkQueuePresentKHR(present_queue_, &present);
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR || window_->was_resized()) {
        window_->clear_resized();
        return recreate_swapchain();
    }
    if (pr != VK_SUCCESS) {
        return std::unexpected(Error::SwapchainFailed);
    }
    current_frame_ = (current_frame_ + 1) % static_cast<std::size_t>(kMaxFramesInFlight);
    return Ok();
}

void Renderer::destroy() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
    for (std::size_t i = 0; i < image_available_semaphores_.size(); ++i) {
        if (device_ == VK_NULL_HANDLE) {
            break;
        }
        vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
        vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
        vkDestroyFence(device_, in_flight_fences_[i], nullptr);
    }
    image_available_semaphores_.clear();
    render_finished_semaphores_.clear();
    in_flight_fences_.clear();

    for (std::size_t i = 0; i < uniform_buffers_.size(); ++i) {
        if (uniform_mapped_[i] != nullptr && device_ != VK_NULL_HANDLE) {
            vkUnmapMemory(device_, uniform_memories_[i]);
        }
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, uniform_buffers_[i], nullptr);
            vkFreeMemory(device_, uniform_memories_[i], nullptr);
        }
    }
    uniform_buffers_.clear();
    uniform_memories_.clear();
    uniform_mapped_.clear();
    descriptor_sets_.clear();

    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }
    command_buffers_.clear();
    cleanup_swapchain();
    if (graphics_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
        graphics_pipeline_ = VK_NULL_HANDLE;
    }
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (debug_messenger_ != VK_NULL_HANDLE) {
        destroy_debug_utils_messenger_ext(instance_, debug_messenger_, nullptr);
        debug_messenger_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    window_ = nullptr;
}

}  // namespace kart::render
