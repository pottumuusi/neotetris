#include "Log.hpp"

const char* g_program_name = "neotetris";

static void game(void)
{
    std::string msg;

    msg = g_program_name;
    msg += " running";
    Log::i(msg);

    msg = g_program_name;
    msg += " shutting down";
    Log::i(msg);
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
