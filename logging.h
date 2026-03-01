#pragma once

#include "stdafx.h"
#include <ctime>
#include <cstdio>

/**
 * SDL3-based logging system with date/time-stamped log rotation
 */

// Initialize the SDL3 logging system with a new log file for each session.
// Sets default log priority to INFO and installs the custom logger.
void initSDL3Logging();

// Cleanup function to properly close log files
void cleanupSDL3Logging();

// Generate a unique log filename (basename only) based on current date/time
char* generateLogFilename();

// Custom SDL3 log output function that writes to timestamped files
void sdl3CustomLogger(void *userdata, int category, SDL_LogPriority priority, const char *message);

// Change the active SDL log priority for SDL_LOG_CATEGORY_APPLICATION.
// Call after argument parsing so the user-requested level takes effect.
void setSDL3LogLevel(SDL_LogPriority priority);

// Parse a log-level string ("verbose", "debug", "info", "warn", "error",
// "critical") into the corresponding SDL_LogPriority.
// Returns SDL_LOG_PRIORITY_INFO and emits a warning for unknown strings.
SDL_LogPriority parseSDL3LogLevel(const char *str);

// Define specific logging levels that map to SDL3's log priorities
#define LOG_VERBOSE(fmt, ...) SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, fmt, ##__VA_ARGS__)
