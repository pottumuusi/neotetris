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

static bool device_is_suitable(const VkPhysicalDevice& device)
{
    return true; // TODO actual suitability check
}

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

    SDL_Window* main_window;

    VkApplicationInfo application_info;
    VkInstanceCreateInfo create_info;

    VkResult result;
    VkInstance vulkan_instance;
    VkPhysicalDevice physical_device;

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

    // SDL createMainRenderer # <- necessary here?

    // SDL setRenderDrawColor # <- necessary here?

    msg_temp = g_program_name;
    msg_temp += " shutting down";
    Log::i(msg_temp);

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
