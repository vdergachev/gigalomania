#pragma once

// pre-compiled header file

#ifdef _WIN32
// get rid of "unsafe" warnings for file commands (fopen vs fopen_s etc)
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS
// SDL3main.lib is not shipped by vcpkg sdl3; we provide WinMain ourselves in main.cpp
#define SDL_MAIN_HANDLED
#endif

#include <vector>
#include <string>
#include <cassert>

// we include SDL globally
#if defined(__MORPHOS__)
#include <SDL/SDL.h>
#else
#include <SDL3/SDL.h>
#endif
#if defined(__APPLE__)
#include <unistd.h>
#endif
