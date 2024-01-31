#include <chrono>
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
    std::uint32_t graphics_family;
};

static struct QueueFamilyIndices
find_queue_family_indices(const VkPhysicalDevice& device)
{
    struct QueueFamilyIndices family_indices;

    std::int32_t i;
    std::uint32_t queue_family_count;
    std::vector<VkQueueFamilyProperties> queue_families;

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
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family_indices.graphics_family = i;
            break;
        }
    }

    if (i == queue_families.size()) {
        g_msg_temp = "Failed to find graphics queue family";
        throw std::runtime_error(g_msg_temp);
    }

    return family_indices;
}

static bool device_is_suitable(const VkPhysicalDevice& device)
{
    bool is_suitable;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    is_suitable = false;
    properties = {};
    features = {};

    vkGetPhysicalDeviceProperties(
            device,
            &properties);
    vkGetPhysicalDeviceFeatures(
            device,
            &features);

    if ( ! features.geometryShader ) {
        return false;
    }

    is_suitable =
        properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
        properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
    if ( ! is_suitable) {
        return false;
    }

    try {
        (void) find_queue_family_indices(device);
    } catch(std::runtime_error const& e) {
        g_msg_temp = "Exception while checking device suitability: ";
        g_msg_temp += e.what();
        Log::w(g_msg_temp);
        return false;
    }

    return true;
}

/*
 * Always selecting the first suitable device which gets found.
 */
static VkPhysicalDevice pick_physical_device(VkInstance* vulkan_instance)
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
        if (device_is_suitable(device)) {
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

    VkResult result;
    VkDevice logical_device;
    VkDeviceQueueCreateInfo queue_create_info;
    VkPhysicalDeviceFeatures device_features;
    VkDeviceCreateInfo device_create_info;

    result = VK_ERROR_UNKNOWN;
    queue_priority = 1.0f;
    logical_device = VK_NULL_HANDLE;
    queue_create_info = {};
    device_features = {};
    device_create_info = {};

    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = family_indices.graphics_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 0;
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

static void game(void)
{
    std::int32_t ret;
    std::uint32_t flags;

    std::uint32_t extension_count;
    std::vector<const char*> extension_names;

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
    VkQueue graphics_queue;

    ret = -1;
    flags = 0;

    extension_count = 0;
    extension_names.resize(0);

    hint_name = SDL_HINT_RENDER_SCALE_QUALITY;
    hint_value = "1";

    window_x = SDL_WINDOWPOS_UNDEFINED;
    window_y = SDL_WINDOWPOS_UNDEFINED;
    window_w = 640;
    window_h = 480;
    window_flags = SDL_WINDOW_SHOWN;
    window_title = g_program_name;

    application_info = {};
    create_info = {};
    const std::string application_name = g_program_name;
    const std::string engine_name = "No engine";

    result = VK_ERROR_UNKNOWN;
    vulkan_instance = {};
    physical_device = VK_NULL_HANDLE;
    logical_device = VK_NULL_HANDLE;

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

    Log::i("Sleeping for 2 seconds");
    std::this_thread::sleep_for(
            std::chrono::milliseconds(2000));

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

    physical_device = pick_physical_device(&vulkan_instance);
    family_indices = find_queue_family_indices(physical_device);
    logical_device = create_logical_device(physical_device, family_indices);
    vkGetDeviceQueue(
            logical_device,
            family_indices.graphics_family,
            0,
            &graphics_queue);

    // SDL createMainRenderer # <- necessary here?

    // SDL setRenderDrawColor # <- necessary here?

    msg_temp = g_program_name;
    msg_temp += " shutting down";
    Log::i(msg_temp);

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
