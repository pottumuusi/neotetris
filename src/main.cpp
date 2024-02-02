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
    std::uint32_t drawing_family;
    std::uint32_t presentation_family;

    std::uint32_t drawing_family_found;
    std::uint32_t presentation_family_found;
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

static bool
device_extensions_are_supported(const VkPhysicalDevice& device)
{
    std::uint32_t i;
    std::uint32_t extension_count;

    std::vector<VkExtensionProperties> available_extensions;

    std::vector<std::string> required_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    std::vector<std::string>::iterator iterator;

    i = 0;
    extension_count = 0;
    available_extensions.resize(0);

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

static bool
device_is_suitable(
        const VkPhysicalDevice& device,
        const VkSurfaceKHR& surface)
{
    bool is_suitable;
    bool extensions_supported;

    struct QueueFamilyIndices family_indices;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    is_suitable = false;
    extensions_supported = false;
    properties = {};
    features = {};
    family_indices = {0};

    Log::i("Checking device suitability");

    vkGetPhysicalDeviceProperties(
            device,
            &properties);
    vkGetPhysicalDeviceFeatures(
            device,
            &features);

    family_indices = find_queue_family_indices(device, surface);
    extensions_supported = device_extensions_are_supported(device);

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

static void
game(void)
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
    VkQueue drawing_queue;
    VkQueue presentation_queue;
    VkSurfaceKHR surface;

    const std::string application_name = g_program_name;
    const std::string engine_name = "No engine";

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

    // SDL createMainRenderer # <- necessary here?

    // SDL setRenderDrawColor # <- necessary here?

    msg_temp = g_program_name;
    msg_temp += " shutting down";
    Log::i(msg_temp);

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
