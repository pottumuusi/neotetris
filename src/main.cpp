#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <thread>
#include <vector>

#include "Log.hpp"

#ifdef __linux__
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif

#ifdef _WIN32
#include <SDL.h>
#endif

// Uncomment to disable debugging
// #define NDEBUG

const char* g_program_name = "neotetris";
std::string g_msg_temp = "";

struct QueueFamilyIndices {
    std::uint32_t drawing_family;
    std::uint32_t presentation_family;

    std::uint32_t drawing_family_found;
    std::uint32_t presentation_family_found;
};

struct SurfaceProperties {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

static struct QueueFamilyIndices
find_queue_family_indices(
        const VkPhysicalDevice& device,
        const VkSurfaceKHR& surface)
{
    struct QueueFamilyIndices family_indices;

    bool set_index;

    std::int32_t i;
    std::uint32_t queue_family_count;
    std::vector<VkQueueFamilyProperties> queue_families;

    VkBool32 has_presentation_support;

    set_index = false;
    i = 0;
    queue_family_count = 0;
    queue_families.resize(0);
    family_indices = {0};

    vkGetPhysicalDeviceQueueFamilyProperties(
            device,
            &queue_family_count,
            nullptr);

    queue_families.resize(queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(
            device,
            &queue_family_count,
            queue_families.data());

    for (i = 0; i < queue_families.size(); i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(
                device,
                i,
                surface,
                &has_presentation_support);

        set_index =
            ( ! family_indices.presentation_family_found) &&
            has_presentation_support;
        if (set_index) {
            // There is a high chance of presentation family being identical to
            // graphics/drawing family. However, attempt to cover more cases by
            // treating presentation family separately.
            family_indices.presentation_family = i;
            family_indices.presentation_family_found = 1;
        }

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family_indices.drawing_family = i;
            family_indices.drawing_family_found = 1;
            break;
        }
    }

    return family_indices;
}

static std::vector<const char*>
get_required_device_extensions()
{
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
}

static bool
device_extensions_are_supported(const VkPhysicalDevice& device)
{
    std::uint32_t i;
    std::uint32_t extension_count;

    std::vector<VkExtensionProperties> available_extensions;

    std::vector<const char*> required_extensions_char;

    std::vector<std::string> required_extensions;
    std::vector<std::string>::iterator iterator;

    i = 0;
    extension_count = 0;
    available_extensions.resize(0);
    required_extensions = {};
    required_extensions_char = get_required_device_extensions();

    for (const char* extension : required_extensions_char) {
        required_extensions.push_back(extension);
    }

    vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &extension_count,
            nullptr);
    available_extensions.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &extension_count,
            available_extensions.data());

    for (const VkExtensionProperties& extension : available_extensions) {
        for (i = 0; i < required_extensions.size(); i++) {
            if (0 == required_extensions[i].compare(extension.extensionName)) {
                iterator = required_extensions.begin() + i;
                required_extensions.erase(iterator);
                break;
            }
        }
    }

    return required_extensions.empty();
}

static struct SurfaceProperties
find_surface_properties(
        const VkPhysicalDevice& device,
        const VkSurfaceKHR& surface)
{
    std::uint32_t count_format;
    std::uint32_t count_mode;

    struct SurfaceProperties surface_properties;

    surface_properties = {0};
    count_format = 0;
    count_mode = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count_format, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count_mode, nullptr);

    if (0 != count_format) {
        surface_properties.formats.resize(count_format);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
                device,
                surface,
                &count_format,
                surface_properties.formats.data());
    }

    if (0 != count_mode) {
        surface_properties.present_modes.resize(count_mode);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                surface,
                &count_mode,
                surface_properties.present_modes.data());
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            device,
            surface,
            &surface_properties.capabilities);

    return surface_properties;
}

static bool
device_is_suitable(
        const VkPhysicalDevice& device,
        const VkSurfaceKHR& surface)
{
    bool is_suitable;
    bool extensions_supported;

    struct QueueFamilyIndices family_indices;
    struct SurfaceProperties surface_properties;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    is_suitable = false;
    extensions_supported = false;
    properties = {};
    features = {};
    family_indices = {0};
    surface_properties = {0};

    Log::i("Checking device suitability");

    vkGetPhysicalDeviceProperties(
            device,
            &properties);
    vkGetPhysicalDeviceFeatures(
            device,
            &features);

    family_indices = find_queue_family_indices(device, surface);
    extensions_supported = device_extensions_are_supported(device);
    surface_properties = find_surface_properties(device, surface);

    if ( ! features.geometryShader ) {
        Log::w("Geometry shader not found");
        return false;
    }

    is_suitable =
        properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
        properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
    if ( ! is_suitable) {
        Log::w("Unsuitable device type");
        return false;
    }

    is_suitable =
        family_indices.drawing_family_found &&
        family_indices.presentation_family_found;
    if ( ! is_suitable) {
        Log::w("Missing family index/indices");
#ifndef NDEBUG
        printf("Family indices:\n");
        printf("\tdrawing_family_found: %u\n",
                family_indices.drawing_family_found);
        printf("\tpresentation_family_found: %u\n",
                family_indices.presentation_family_found);
#endif // NDEBUG
        return false;
    }

    if ( ! extensions_supported) {
        Log::w("Required extensions not supported");
        return false;
    }

    // Via surface properties, check if swap chain support is present.
    is_suitable =
        ( ! surface_properties.formats.empty()) &&
        ( ! surface_properties.present_modes.empty());
    if ( ! is_suitable) {
        Log::w("Insufficient swap chain support");
        return false;
    }

    return true;
}

static VkSurfaceKHR
create_surface(SDL_Window* window, VkInstance instance)
{
    SDL_bool ret;
    VkSurfaceKHR surface;

    surface = VK_NULL_HANDLE;
    ret = SDL_FALSE;

    ret = SDL_Vulkan_CreateSurface(window, instance, &surface);
    if (SDL_FALSE == ret) {
        g_msg_temp = "Failed to create surface: ";
        g_msg_temp += SDL_GetError();
        throw std::runtime_error(g_msg_temp);
    }

    return surface;
}

/*
 * Always selecting the first suitable device which gets found.
 */
static VkPhysicalDevice
pick_physical_device(VkInstance* vulkan_instance, const VkSurfaceKHR& surface)
{
    std::uint32_t device_count;

    std::vector<VkPhysicalDevice> devices;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    device_count = 0;
    physical_device = VK_NULL_HANDLE;
    properties = {};
    features = {};
    devices.resize(0);

    vkEnumeratePhysicalDevices(*vulkan_instance, &device_count, nullptr);
    if (0 == device_count) {
        g_msg_temp = "Failed to find devices with Vulkan support";
        throw std::runtime_error(g_msg_temp);
    }

    devices.resize(device_count);

    vkEnumeratePhysicalDevices(*vulkan_instance, &device_count, devices.data());

#ifndef NDEBUG
    for (const VkPhysicalDevice& device : devices) {
        vkGetPhysicalDeviceProperties(
                device,
                &properties);
        vkGetPhysicalDeviceFeatures(
                device,
                &features);

        std::cout << "Enumerated physical devices:" << "\n";
        std::cout << "\t" << properties.deviceName << "\n";
        std::cout << "\t" << properties.deviceType << "\n";
        std::cout << "\t" << "Geometry Shader available: "
            << features.geometryShader << "\n";
    }
#endif

    for (const VkPhysicalDevice& device : devices) {
        if (device_is_suitable(device, surface)) {
            physical_device = device;
            break;
        }
    }

    if (VK_NULL_HANDLE == physical_device) {
        g_msg_temp = "No suitable device found";
        throw std::runtime_error(g_msg_temp);
    }

    return physical_device;
}

static VkDevice
create_logical_device(
        const VkPhysicalDevice& device,
        const struct QueueFamilyIndices& family_indices)
{
    float queue_priority;

    std::vector<const char*> required_extensions;

    VkResult result;
    VkDevice logical_device;
    VkDeviceQueueCreateInfo queue_create_info;
    std::vector<VkDeviceQueueCreateInfo> queue_create_info_vector;
    VkPhysicalDeviceFeatures device_features;
    VkDeviceCreateInfo device_create_info;

    result = VK_ERROR_UNKNOWN;
    queue_priority = 1.0f;
    logical_device = VK_NULL_HANDLE;
    queue_create_info = {};
    device_features = {};
    device_create_info = {};
    required_extensions = get_required_device_extensions();

    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = family_indices.drawing_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_info_vector.push_back(queue_create_info);

    queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = family_indices.presentation_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_info_vector.push_back(queue_create_info);

    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_info_vector.data();
    device_create_info.queueCreateInfoCount = queue_create_info_vector.size();
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.ppEnabledExtensionNames = required_extensions.data();
    device_create_info.enabledExtensionCount = required_extensions.size();
    device_create_info.enabledLayerCount = 0;

    result = vkCreateDevice(
            device,
            &device_create_info,
            nullptr,
            &logical_device);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create logical device");
    }

    return logical_device;
}

static VkSurfaceFormatKHR
pick_swap_surface_format(
        const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    bool should_pick;

    should_pick = false;

    for (const VkSurfaceFormatKHR& format : available_formats) {
        should_pick =
            VK_FORMAT_B8G8R8A8_SRGB == format.format &&
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == format.colorSpace;
        if (should_pick) {
            return format;
        }
    }

    Log::w("Could not find the preferred format combination");

    return available_formats[0];
}

static VkPresentModeKHR
pick_swap_present_mode(std::vector<VkPresentModeKHR> present_modes)
{
    for (const VkPresentModeKHR& mode : present_modes) {
        if (VK_PRESENT_MODE_FIFO_KHR == mode) {
            // VK_PRESENT_MODE_FIFO_KHR is told to be quaranteed to be
            // available.
            return VK_PRESENT_MODE_FIFO_KHR;
        }
    }

    throw std::runtime_error("VK_PRESENT_MODE_FIFO_KHR not available");
}

static VkExtent2D
pick_swap_extent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        SDL_Window* window)
{
    int pixels_drawable_height;
    int pixels_drawable_width;

    VkExtent2D extent;

    extent = {};

    // Special (rare) value uint32 max tells that swap chain resolution can
    // differ from window resolution.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    SDL_Vulkan_GetDrawableSize(
            window,
            &pixels_drawable_width,
            &pixels_drawable_height);

    extent = {
        static_cast<uint32_t>(pixels_drawable_width),
        static_cast<uint32_t>(pixels_drawable_height)
    };

    extent.width = std::clamp(
            extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
    extent.height = std::clamp(
            extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

    return extent;
}

static VkSwapchainKHR
create_swap_chain(
        const VkPhysicalDevice& device,
        const VkDevice& logical_device,
        const VkSurfaceKHR& surface,
        SDL_Window* window,
        VkFormat& swapchain_image_format,
        VkExtent2D& swapchain_extent)
{
    bool condition;
    std::uint32_t image_count;

    std::uint32_t family_index_count;
    const std::uint32_t* family_index_array_base;

    struct QueueFamilyIndices family_indices;
    struct SurfaceProperties surface_properties;

    VkResult result;

    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;

    VkSharingMode sharing_mode;

    VkSwapchainKHR swapchain;
    VkSwapchainCreateInfoKHR chain_create_info;

    condition = false;
    image_count = 0;

    sharing_mode = VK_SHARING_MODE_MAX_ENUM;

    family_index_count = 0;

    result = VK_ERROR_UNKNOWN;
    family_indices = {0};
    surface_properties = {0};
    format = {};
    present_mode = {};
    extent = {};
    swapchain = {};
    chain_create_info = {};

    std::uint32_t family_index_array[2] = {
        family_indices.drawing_family,
        family_indices.presentation_family,
    };

    surface_properties = find_surface_properties(device, surface);
    format = pick_swap_surface_format(surface_properties.formats);
    present_mode = pick_swap_present_mode(surface_properties.present_modes);
    extent = pick_swap_extent(surface_properties.capabilities, window);
    family_indices = find_queue_family_indices(device, surface);

    sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    family_index_count = 0;
    family_index_array_base = nullptr;
    condition =
        family_indices.drawing_family !=
        family_indices.presentation_family;
    if (condition) {
        // If drawing and presentation have separate queues, then using
        // concurrent mode to avoid writing image ownership transfer code.
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
        family_index_count = 2;
        family_index_array_base = family_index_array;
    }

    image_count = surface_properties.capabilities.minImageCount + 1;

    // 0 maxImageCount stands for 'no maximum'
    condition =
        surface_properties.capabilities.maxImageCount > 0 &&
        image_count > surface_properties.capabilities.maxImageCount;
    if (condition) {
        image_count = surface_properties.capabilities.maxImageCount;
    }

    chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    chain_create_info.surface = surface;
    chain_create_info.minImageCount = image_count;
    chain_create_info.imageFormat = format.format;
    chain_create_info.imageColorSpace = format.colorSpace;
    chain_create_info.imageExtent = extent;
    chain_create_info.imageArrayLayers = 1;
    chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    chain_create_info.imageSharingMode = sharing_mode;
    chain_create_info.queueFamilyIndexCount = family_index_count;
    chain_create_info.pQueueFamilyIndices = family_index_array_base;

    // Specifying currentTransform in order to get no transform
    chain_create_info.preTransform =
        surface_properties.capabilities.currentTransform;
    chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    chain_create_info.presentMode = present_mode;
    chain_create_info.clipped = VK_TRUE;
    chain_create_info.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(
            logical_device,
            &chain_create_info,
            nullptr,
            &swapchain);
    if (VK_SUCCESS != result) {
        g_msg_temp = "Failed to create swapchain: ";
        g_msg_temp += result;
        throw std::runtime_error(g_msg_temp);
    }

    swapchain_image_format = format.format;
    swapchain_extent = extent;

    return swapchain;
}

static std::vector<VkImageView>
create_image_views(
        const VkDevice& logical_device,
        const std::vector<VkImage>& swapchain_images,
        const VkFormat& swapchain_image_format)
{
    VkImageViewCreateInfo create_info;
    std::vector<VkImageView> image_views;

    VkResult result;

    create_info = {};
    image_views.resize(0);
    result = VK_ERROR_UNKNOWN;

    image_views.resize(swapchain_images.size());

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        result = vkCreateImageView(
                logical_device,
                &create_info,
                nullptr,
                &image_views[i]);
        if (VK_SUCCESS != result) {
            throw std::runtime_error("Failed to create image view");
        }
    }

    return image_views;
}

static std::vector<char>
read_file(const std::string& filename)
{
    size_t file_size;

    std::vector<char> buffer;

    file_size = 0;
    buffer.resize(0);

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if ( ! file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    file_size = (size_t) file.tellg();
    buffer.resize(file_size);

    g_msg_temp = "Reading file with size of: ";
    g_msg_temp += std::to_string(file_size);
    Log::d(g_msg_temp);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

static VkShaderModule
create_shader_module(
        const VkDevice& logical_device,
        const std::vector<char>& code)
{
    VkResult result;

    VkShaderModuleCreateInfo create_info;
    VkShaderModule shader_module;

    create_info = {};
    shader_module = nullptr;
    result = VK_ERROR_UNKNOWN;

    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    result = vkCreateShaderModule(logical_device, &create_info, nullptr, &shader_module);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shader_module;
}

static VkRenderPass
create_render_pass(
        const VkDevice& logical_device,
        const VkFormat swapchain_image_format)
{
    VkResult result;
    VkRenderPass render_pass;
    VkSubpassDescription subpass;
    VkSubpassDependency dependency;
    VkRenderPassCreateInfo render_pass_info;
    VkAttachmentDescription color_attachment;
    VkAttachmentReference color_attachment_ref;

    result = VK_ERROR_UNKNOWN;
    subpass = {};
    dependency = {};
    render_pass = {};
    render_pass_info = {};
    color_attachment = {};
    color_attachment_ref = {};

    color_attachment.format = swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    result = vkCreateRenderPass(
            logical_device,
            &render_pass_info,
            nullptr,
            &render_pass);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create render pass");
    }

    return render_pass;
}

static VkPipelineLayout
create_pipeline_layout(const VkDevice& logical_device)
{
    VkResult result;

    VkPipelineLayout pipeline_layout;
    VkPipelineLayoutCreateInfo pipeline_layout_info;

    result = VK_ERROR_UNKNOWN;

    pipeline_layout = {};
    pipeline_layout_info = {};

    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(
            logical_device,
            &pipeline_layout_info,
            nullptr,
            &pipeline_layout);
    if (VK_SUCCESS != result) {
        std::runtime_error("Failed to create pipeline layout");
    }

    return pipeline_layout;
}

static VkViewport
get_viewport(const VkExtent2D& swapchain_extent)
{
    VkViewport viewport;

    viewport = {};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}


static VkRect2D
get_scissor(const VkExtent2D& swapchain_extent)
{
    VkRect2D scissor;

    scissor = {};

    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;

    return scissor;
}

static VkPipeline
create_graphics_pipeline(
        const VkDevice& logical_device,
        const VkExtent2D& swapchain_extent,
        const VkPipelineLayout& pipeline_layout,
        const VkRenderPass& render_pass)
{
    std::string entrypoint = "main";
    std::vector<char> shader_code_vert;
    std::vector<char> shader_code_frag;
    VkShaderModule shader_module_vert;
    VkShaderModule shader_module_frag;

    VkResult result;

    VkPipelineShaderStageCreateInfo shader_stage_info_vert;
    VkPipelineShaderStageCreateInfo shader_stage_info_frag;
    VkPipelineShaderStageCreateInfo shader_stages[2];

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineColorBlendAttachmentState color_blend_attachment;
    VkPipelineColorBlendStateCreateInfo color_blending;
    VkPipelineDynamicStateCreateInfo dynamic_state;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkViewport viewport;
    VkRect2D scissor;

    VkPipeline graphics_pipeline;
    VkGraphicsPipelineCreateInfo pipeline_info;

    const std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    shader_stage_info_vert = {};
    shader_stage_info_frag = {};
    color_blend_attachment = {};
    graphics_pipeline = {};
    vertex_input_info = {};
    input_assembly = {};
    viewport_state = {};
    color_blending = {};
    dynamic_state = {};
    multisampling = {};
    pipeline_info = {};
    rasterizer = {};
    viewport = {};
    scissor = {};

    result = VK_ERROR_UNKNOWN;

    shader_code_vert = read_file("shaders/vert.spv");
    shader_code_frag = read_file("shaders/frag.spv");

    shader_module_vert = create_shader_module(
            logical_device,
            shader_code_vert);
    shader_module_frag = create_shader_module(
            logical_device,
            shader_code_frag);

    shader_stage_info_vert.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info_vert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_info_vert.module = shader_module_vert;
    shader_stage_info_vert.pName = entrypoint.c_str();

    shader_stage_info_frag.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info_frag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_info_frag.module = shader_module_frag;
    shader_stage_info_frag.pName = entrypoint.c_str();

    shader_stages[0] = shader_stage_info_vert;
    shader_stages[1] = shader_stage_info_frag;

    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    viewport = get_viewport(swapchain_extent);

    scissor = get_scissor(swapchain_extent);

    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(
            logical_device,
            VK_NULL_HANDLE,
            1,
            &pipeline_info,
            nullptr,
            &graphics_pipeline);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(logical_device, shader_module_vert, nullptr);
    vkDestroyShaderModule(logical_device, shader_module_frag, nullptr);

    return graphics_pipeline;
}

static std::vector<VkFramebuffer>
create_framebuffers(
        const VkDevice& logical_device,
        const std::vector<VkImageView>& swapchain_image_views,
        const VkRenderPass& render_pass,
        const VkExtent2D swapchain_extent)
{
    VkResult result;

    std::vector<VkFramebuffer> framebuffers;

    VkFramebufferCreateInfo framebuffer_info;
    VkImageView attachments[1];

    result = VK_ERROR_UNKNOWN;
    framebuffer_info = {};
    framebuffers.resize(0);
    attachments[0] = VK_NULL_HANDLE;

    framebuffers.resize(swapchain_image_views.size());

    for (size_t i = 0; i < swapchain_image_views.size(); i++) {
        attachments[0] = swapchain_image_views[i];

        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swapchain_extent.width;
        framebuffer_info.height = swapchain_extent.height;
        framebuffer_info.layers = 1;

        result = vkCreateFramebuffer(
                logical_device,
                &framebuffer_info,
                nullptr,
                &framebuffers[i]);
        if (VK_SUCCESS != result) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }

    return framebuffers;
}

static VkCommandPool
create_command_pool(
        const VkDevice& logical_device,
        const struct QueueFamilyIndices& family_indices)
{
    VkResult result;
    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_info;

    result = VK_ERROR_UNKNOWN;
    pool_info = {};

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = family_indices.drawing_family;

    result = vkCreateCommandPool(
            logical_device,
            &pool_info,
            nullptr,
            &command_pool);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create command pool");
    }

    return command_pool;
}

static VkCommandBuffer
create_command_buffer(
        const VkDevice& logical_device,
        const VkCommandPool& command_pool)
{
    VkResult result;
    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo allocate_info;

    result = VK_ERROR_UNKNOWN;
    allocate_info = {};

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(
            logical_device,
            &allocate_info,
            &command_buffer);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to allocate command buffers");;
    }

    return command_buffer;
}

static void
record_command_buffer(
        VkCommandBuffer command_buffer,
        uint32_t image_index,
        VkRenderPass& render_pass,
        std::vector<VkFramebuffer>& swapchain_framebuffers,
        VkExtent2D& swapchain_extent,
        VkPipeline& graphics_pipeline)
{
    VkResult result;
    VkCommandBufferBeginInfo begin_info;
    VkRenderPassBeginInfo render_pass_info;
    VkClearValue clear_color;
    VkViewport viewport;
    VkRect2D scissor;

    result = VK_ERROR_UNKNOWN;
    scissor = {};
    viewport = {};
    begin_info = {};
    render_pass_info = {};
    clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to begin recording a command buffer");
    }

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swapchain_framebuffers[image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    viewport = get_viewport(swapchain_extent);
    scissor = get_scissor(swapchain_extent);

    vkCmdBeginRenderPass(
            command_buffer,
            &render_pass_info,
            VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline);

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to end command buffer");
    }
}

static void
draw_frame(
        VkDevice& logical_device,
        VkFence& fence_in_flight,
        VkSwapchainKHR& swapchain,
        VkSemaphore& semaphore_image_available,
        VkSemaphore& semaphore_render_finished,
        VkCommandBuffer& command_buffer,
        VkRenderPass& render_pass,
        std::vector<VkFramebuffer>& swapchain_framebuffers,
        VkExtent2D& swapchain_extent,
        VkPipeline& graphics_pipeline,
        VkQueue& drawing_queue,
        VkQueue& presentation_queue)
{
    VkResult result;
    VkSubmitInfo submit_info;
    VkPresentInfoKHR present_info;
    VkSemaphore wait_semaphores[1];
    VkSemaphore signal_semaphores[1];
    VkPipelineStageFlags wait_stages[1];
    VkSwapchainKHR swapchains[1];

    uint32_t image_index;
    uint32_t flags;

    result = VK_ERROR_UNKNOWN;
    submit_info = {};
    present_info = {};
    swapchains[0] = swapchain;
    wait_semaphores[0] = semaphore_image_available;
    signal_semaphores[0] = semaphore_render_finished;
    wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    flags = 0;
    image_index = -1;

    vkWaitForFences(logical_device, 1, &fence_in_flight, VK_TRUE, UINT64_MAX);

    vkResetFences(logical_device, 1, &fence_in_flight);

    // TODO handle return value
    (void) vkAcquireNextImageKHR(
            logical_device,
            swapchain,
            UINT64_MAX,
            semaphore_image_available,
            VK_NULL_HANDLE,
            &image_index);

    vkResetCommandBuffer(command_buffer, flags);

    record_command_buffer(
            command_buffer,
            image_index,
            render_pass,
            swapchain_framebuffers,
            swapchain_extent,
            graphics_pipeline);

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    result = vkQueueSubmit(drawing_queue, 1, &submit_info, fence_in_flight);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    (void) vkQueuePresentKHR(presentation_queue, &present_info);
}

static VkSemaphore
create_vulkan_semaphore(const VkDevice& logical_device)
{
    VkResult result;
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_info;

    result = VK_ERROR_UNKNOWN;
    semaphore = {};
    semaphore_info = {};

    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &semaphore);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create vulkan semaphore");
    }

    return semaphore;
}

static VkFence
create_vulkan_fence(const VkDevice& logical_device)
{
    VkResult result;
    VkFence fence;
    VkFenceCreateInfo fence_info;

    result = VK_ERROR_UNKNOWN;
    fence = {};
    fence_info = {};

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = vkCreateFence(logical_device, &fence_info, nullptr, &fence);
    if (VK_SUCCESS != result) {
        throw std::runtime_error("Failed to create vulkan fence");
    }

    return fence;
}

static void
game(void)
{
    std::int32_t ret;
    std::uint32_t flags;

    std::uint32_t extension_count;
    std::vector<const char*> extension_names;
    std::vector<VkFramebuffer> swapchain_framebuffers;

    std::string msg_temp; // TODO start using g_msg_temp instead
    std::string hint_name;
    std::string hint_value;

    std::int32_t window_x;
    std::int32_t window_y;
    std::int32_t window_w;
    std::int32_t window_h;
    std::uint32_t window_flags;
    std::string window_title;

    struct QueueFamilyIndices family_indices;

    SDL_Window* main_window;

    VkApplicationInfo application_info;
    VkInstanceCreateInfo create_info;

    VkResult result;
    VkInstance vulkan_instance;
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    VkQueue drawing_queue;
    VkQueue presentation_queue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore semaphore_image_available;
    VkSemaphore semaphore_render_finished;
    VkFence fence_in_flight;

    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkImage> swapchain_images;
    std::uint32_t swapchain_image_count;

    const std::string application_name = g_program_name;
    const std::string engine_name = "No engine";

    ret = -1;
    flags = 0;

    extension_count = 0;
    extension_names.resize(0);
    swapchain_framebuffers.resize(0);

    hint_name = SDL_HINT_RENDER_SCALE_QUALITY;
    hint_value = "1";

    window_x = SDL_WINDOWPOS_UNDEFINED;
    window_y = SDL_WINDOWPOS_UNDEFINED;
    window_w = 640;
    window_h = 480;
    window_flags = SDL_WINDOW_VULKAN;
    window_title = g_program_name;

    application_info = {};
    create_info = {};

    result = VK_ERROR_UNKNOWN;
    vulkan_instance = {};
    physical_device = VK_NULL_HANDLE;
    logical_device = VK_NULL_HANDLE;
    drawing_queue = VK_NULL_HANDLE;
    presentation_queue = VK_NULL_HANDLE;
    surface = VK_NULL_HANDLE;
    swapchain = VK_NULL_HANDLE;
    swapchain_image_format = VK_FORMAT_UNDEFINED;
    swapchain_extent = {};
    render_pass = {};
    pipeline_layout = {};
    graphics_pipeline = {};
    command_pool = {};

    swapchain_image_views.resize(0);
    swapchain_images.resize(0);
    swapchain_image_count = 0;

    family_indices = {0};

    msg_temp = g_program_name;
    msg_temp += " running";
    Log::i(msg_temp);

    // SDL init
    flags = SDL_INIT_VIDEO;
    ret = SDL_Init(flags);
    if (ret < 0) {
        msg_temp = "Failed to initialize SDL: ";
        msg_temp += SDL_GetError();
        throw std::runtime_error(msg_temp);
    }

    // SDL setHint
    ret = SDL_SetHint(hint_name.c_str(), hint_value.c_str());
    if (ret < 0) {
        msg_temp = "Setting SDL hint";
        msg_temp += hint_name;
        msg_temp += " failed";
        Log::w(msg_temp);
    }

    // SDL imgInit # <- necessary here?

    // SDL createMainWindow
    main_window = SDL_CreateWindow(
            window_title.c_str(),
            window_x,
            window_y,
            window_w,
            window_h,
            window_flags);
    if (NULL == main_window) {
        msg_temp = "Failed to create window: ";
        msg_temp += SDL_GetError();
        throw std::runtime_error(msg_temp);
    }

    Log::i("Sleeping for 1 second");
    std::this_thread::sleep_for(
            std::chrono::milliseconds(1000));

    SDL_Vulkan_GetInstanceExtensions(main_window, &extension_count, nullptr);
    extension_names.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(main_window, &extension_count, extension_names.data());

    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = application_name.c_str();
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    application_info.pEngineName = engine_name.c_str();
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    application_info.apiVersion = VK_API_VERSION_1_0;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(
            extension_names.size());
    create_info.ppEnabledExtensionNames = extension_names.data();
    create_info.enabledLayerCount = 0; // Default to no validation layers

    result = vkCreateInstance(&create_info, nullptr, &vulkan_instance);
    if (VK_SUCCESS != result) {
        msg_temp = "Failed to create Vulkan instance";
        throw std::runtime_error(msg_temp);
    }

    surface = create_surface(main_window, vulkan_instance);
    physical_device = pick_physical_device(&vulkan_instance, surface);
    family_indices = find_queue_family_indices(physical_device, surface);
    logical_device = create_logical_device(physical_device, family_indices);
    vkGetDeviceQueue(
            logical_device,
            family_indices.drawing_family,
            0,
            &drawing_queue);
    vkGetDeviceQueue(
            logical_device,
            family_indices.presentation_family,
            0,
            &presentation_queue);

    swapchain = create_swap_chain(
            physical_device,
            logical_device,
            surface,
            main_window,
            swapchain_image_format,
            swapchain_extent);

    vkGetSwapchainImagesKHR(
            logical_device,
            swapchain,
            &swapchain_image_count,
            nullptr);
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(
            logical_device,
            swapchain,
            &swapchain_image_count,
            swapchain_images.data());

    swapchain_image_views = create_image_views(
            logical_device,
            swapchain_images,
            swapchain_image_format);

    render_pass = create_render_pass(
            logical_device,
            swapchain_image_format);

    pipeline_layout = create_pipeline_layout(logical_device);

    Log::i("Creating Vulkan graphics pipeline");
    graphics_pipeline = create_graphics_pipeline(
            logical_device,
            swapchain_extent,
            pipeline_layout,
            render_pass);

    swapchain_framebuffers = create_framebuffers(
            logical_device,
            swapchain_image_views,
            render_pass,
            swapchain_extent);

    command_pool = create_command_pool(
            logical_device,
            family_indices);

    command_buffer = create_command_buffer(
            logical_device,
            command_pool);

    semaphore_image_available = create_vulkan_semaphore(logical_device);
    semaphore_render_finished = create_vulkan_semaphore(logical_device);
    fence_in_flight = create_vulkan_fence(logical_device);

#if 0
    draw_frame(
            logical_device,
            fence_in_flight,
            swapchain,
            semaphore_image_available,
            semaphore_render_finished,
            command_buffer,
            render_pass,
            swapchain_framebuffers,
            swapchain_extent,
            graphics_pipeline,
            drawing_queue,
            presentation_queue);
#endif

#if 0
    // TODO Main loop here
    while (1 /* loop until window close event */) {
    }
#endif

    Log::i("Sleeping for 1 second");
    std::this_thread::sleep_for(
            std::chrono::milliseconds(1000));

    // SDL createMainRenderer # <- necessary here?

    // SDL setRenderDrawColor # <- necessary here?

    msg_temp = g_program_name;
    msg_temp += " shutting down";
    Log::i(msg_temp);

    vkDestroySemaphore(logical_device, semaphore_image_available, nullptr);
    vkDestroySemaphore(logical_device, semaphore_render_finished, nullptr);
    vkDestroyFence(logical_device, fence_in_flight, nullptr);

    vkDestroyCommandPool(logical_device, command_pool, nullptr);

    for (VkFramebuffer framebuffer : swapchain_framebuffers) {
        vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(logical_device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
    vkDestroyRenderPass(logical_device, render_pass, nullptr);

    for (VkImageView image_view : swapchain_image_views) {
        vkDestroyImageView(logical_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(logical_device, swapchain, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyDevice(logical_device, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);

    SDL_Quit();
}

// Disable name mangling so that SDL can find and redefine main.
// https://djrollins.com/2016/10/02/sdl-on-windows/
extern "C" int main(int argc, char* argv[])
{
    try {
        game();
    } catch(std::exception const& e) {
        std::string msg = "Terminating due to unhandled exception: ";
        msg += e.what();
        Log::e(msg);
    }

    return 0;
}
