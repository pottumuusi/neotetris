#include <chrono>
#include <thread>

#include "Log.hpp"

#ifdef __linux__
#include <SDL2/SDL.h>
#endif

#ifdef _WIN32
#include <SDL.h>
#endif

const char* g_program_name = "neotetris";

static void game(void)
{
    std::int32_t ret;
    std::uint32_t flags;

    std::string msg_temp;
    std::string hint_name;
    std::string hint_value;

    std::int32_t window_x;
    std::int32_t window_y;
    std::int32_t window_w;
    std::int32_t window_h;
    std::uint32_t window_flags;
    std::string window_title;

    SDL_Window* main_window;

    ret = -1;
    flags = 0;

    hint_name = SDL_HINT_RENDER_SCALE_QUALITY;
    hint_value = "1";

    window_x = SDL_WINDOWPOS_UNDEFINED;
    window_y = SDL_WINDOWPOS_UNDEFINED;
    window_w = 640;
    window_h = 480;
    window_flags = SDL_WINDOW_SHOWN;
    window_title = g_program_name;

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

    // SDL createMainRenderer # <- necessary here?

    // SDL setRenderDrawColor # <- necessary here?

    msg_temp = g_program_name;
    msg_temp += " shutting down";
    Log::i(msg_temp);

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
