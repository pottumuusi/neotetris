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

    std::string msg;

    ret = -1;
    flags = 0;

    msg = g_program_name;
    msg += " running";
    Log::i(msg);

    // SDL init
    flags = SDL_INIT_VIDEO;
    ret = SDL_Init(flags);

    // SDL setHint

    // SDL imgInit # <- necessary here?

    // SDL createMainWindow

    // SDL createMainRenderer # <- necessary here?

    // SDL setRenderDrawColor # <- necessary here?

    msg = g_program_name;
    msg += " shutting down";
    Log::i(msg);

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
