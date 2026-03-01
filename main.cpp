#include "stdafx.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "game.h"

#ifdef _WIN32
#include <SDL3/SDL_main.h>
// WinMain entry point for SUBSYSTEM:WINDOWS; SDL3main.lib not shipped by vcpkg.
// SDL_MAIN_HANDLED is defined in stdafx.h so SDL does not redefine main.
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrev*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    SDL_SetMainReady();
    playGame(0, nullptr);
    return 0;
}
#else
int main(int argc, char *argv[])
{
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "started main");
#endif

    playGame(argc, argv);

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "about to exit main");
#endif
    return 0;
}
#endif
