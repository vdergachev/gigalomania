#include "stdafx.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "game.h"
#include "logging.h"

#ifdef _WIN32
#include <SDL3/SDL_main.h>
// WinMain entry point for SUBSYSTEM:WINDOWS; SDL3main.lib not shipped by vcpkg.
// SDL_MAIN_HANDLED is defined in stdafx.h so SDL does not redefine main.
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrev*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    SDL_SetMainReady();
    // Initialize SDL3 logging system
    initSDL3Logging();
    
    // Parse command line into argc/argv (WinMain does not receive them directly)
    int argc = 0;
    LPWSTR *argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = nullptr;
    if (argvW != nullptr && argc > 0) {
        argv = new char*[argc];
        for (int i = 0; i < argc; i++) {
            int len = WideCharToMultiByte(CP_ACP, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
            argv[i] = new char[len];
            WideCharToMultiByte(CP_ACP, 0, argvW[i], -1, argv[i], len, nullptr, nullptr);
        }
        LocalFree(argvW);
    }
    playGame(argc, argv);
    if (argv != nullptr) {
        for (int i = 0; i < argc; i++) delete[] argv[i];
        delete[] argv;
    }
    
    // Cleanup SDL3 logging system
    cleanupSDL3Logging();
    
    return 0;
}
#else
int main(int argc, char *argv[])
{
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "started main");
#endif

    // Initialize SDL3 logging system
    initSDL3Logging();

    playGame(argc, argv);

    // Cleanup SDL3 logging system
    cleanupSDL3Logging();

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "about to exit main");
#endif
    return 0;
}
#endif
