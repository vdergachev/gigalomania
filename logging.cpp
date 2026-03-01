#include "stdafx.h"
#include "logging.h"
#include "utils.h"
#include "common.h"
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

static FILE* logFile = nullptr;
static char* currentLogFilename = nullptr;

// Maximum number of log files to keep in the logs directory
static const int MAX_LOG_FILES_c = 2;

// Generate a unique log filename (basename only, no path)
char* generateLogFilename() {
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[100];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format: gigalomania_YYYY-MM-DD_HH-MM-SS.log
    strftime(buffer, sizeof(buffer), "gigalomania_%Y-%m-%d_%H-%M-%S.log", timeinfo);

    char* filename = new char[strlen(buffer) + 1];
    strcpy(filename, buffer);
    return filename;
}

// Callback userdata for log file enumeration
struct LogEnumData {
    std::string dir;
    std::vector<std::string> files;
};

// SDL_EnumerateDirectory callback - collects *.log filenames
static SDL_EnumerationResult collectLogFiles(void *userdata, const char *dirname, const char *fname) {
    if (!fname) return SDL_ENUM_CONTINUE;
    size_t len = strlen(fname);
    if (len > 4 && strcmp(fname + len - 4, ".log") == 0) {
        LogEnumData *data = static_cast<LogEnumData*>(userdata);
        data->files.push_back(std::string(fname));
    }
    return SDL_ENUM_CONTINUE;
}

// Remove oldest log files if the count exceeds MAX_LOG_FILES_c.
// Filenames are sorted lexicographically; since names embed timestamps, the
// oldest files sort first.
static void enforceLogRotation(const std::string &logs_dir) {
    LogEnumData data;
    data.dir = logs_dir;

    if (!SDL_EnumerateDirectory(logs_dir.c_str(), collectLogFiles, &data)) {
        return; // enumeration failed - skip rotation
    }

    // Keep only MAX_LOG_FILES_c - 1 existing files so that the new file we
    // are about to create brings the total to exactly MAX_LOG_FILES_c.
    if ((int)data.files.size() < MAX_LOG_FILES_c) {
        return;
    }

    std::sort(data.files.begin(), data.files.end()); // oldest timestamps first

    int to_delete = (int)data.files.size() - (MAX_LOG_FILES_c - 1);
    for (int i = 0; i < to_delete; i++) {
        std::string full = logs_dir + data.files[i];
        SDL_RemovePath(full.c_str());
    }
}

// Custom SDL3 log output function that writes to timestamped files
void sdl3CustomLogger(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    if (!message) return;

    // Convert SDL log priority to string
    const char *priorityStr = "UNKNOWN";
    switch (priority) {
        case SDL_LOG_PRIORITY_VERBOSE:  priorityStr = "VERBOSE"; break;
        case SDL_LOG_PRIORITY_DEBUG:    priorityStr = "DEBUG";   break;
        case SDL_LOG_PRIORITY_INFO:     priorityStr = "INFO";    break;
        case SDL_LOG_PRIORITY_WARN:     priorityStr = "WARN";    break;
        case SDL_LOG_PRIORITY_ERROR:    priorityStr = "ERROR";   break;
        case SDL_LOG_PRIORITY_CRITICAL: priorityStr = "CRITICAL"; break;
        case SDL_LOG_PRIORITY_INVALID:
        case SDL_LOG_PRIORITY_TRACE:
        case SDL_LOG_PRIORITY_COUNT:
        default: priorityStr = "UNKNOWN"; break;
    }

    // Convert SDL log category to string
    const char *categoryStr = "UNKNOWN";
    switch (static_cast<SDL_LogCategory>(category)) {
        case SDL_LOG_CATEGORY_APPLICATION: categoryStr = "APP";    break;
        case SDL_LOG_CATEGORY_ERROR:       categoryStr = "ERROR";  break;
        case SDL_LOG_CATEGORY_ASSERT:      categoryStr = "ASSERT"; break;
        case SDL_LOG_CATEGORY_SYSTEM:      categoryStr = "SYSTEM"; break;
        case SDL_LOG_CATEGORY_AUDIO:       categoryStr = "AUDIO";  break;
        case SDL_LOG_CATEGORY_VIDEO:       categoryStr = "VIDEO";  break;
        case SDL_LOG_CATEGORY_RENDER:      categoryStr = "RENDER"; break;
        case SDL_LOG_CATEGORY_INPUT:       categoryStr = "INPUT";  break;
        case SDL_LOG_CATEGORY_TEST:        categoryStr = "TEST";   break;
        default:                           categoryStr = "CUSTOM"; break;
    }

    // Get current time for timestamp
    time_t rawtime;
    struct tm* timeinfo;
    char timeBuffer[20];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Format the log message
    char logMessage[2048];
    snprintf(logMessage, sizeof(logMessage), "[%s][%s][%s] %s\n",
             timeBuffer, categoryStr, priorityStr, message);

    // Write to log file if available
    if (logFile) {
        fputs(logMessage, logFile);
        fflush(logFile); // ensure immediate write
    }

    // Also print to stdout/stderr based on priority
    if (priority >= SDL_LOG_PRIORITY_WARN) {
        fprintf(stderr, "%s", logMessage);
    } else {
        printf("%s", logMessage);
    }

#if defined(__ANDROID__)
    // Also send to Android log system
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "%s", logMessage);
#endif
}

// Initialize the SDL3 logging system with a new log file for each session.
// Opens a timestamped file under SDL_GetPrefPath()/logs/ and applies rolling
// rotation keeping at most MAX_LOG_FILES_c files.
void initSDL3Logging() {
    // Install our custom logger first so even fallback SDL_Log calls go there
    SDL_SetLogOutputFunction(sdl3CustomLogger, nullptr);

    // Set default log level to INFO - banner lines are visible, debug traces are not
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    // Generate session filename (basename)
    currentLogFilename = generateLogFilename();

    // Determine logs directory via SDL_GetPrefPath
    std::string logs_dir;
    char *pref_path = SDL_GetPrefPath("Gigalomania", "Gigalomania");
    if (pref_path) {
        logs_dir = std::string(pref_path) + "logs/";
        SDL_free(pref_path);
        pref_path = nullptr;

        // Create logs/ subdirectory (SDL_CreateDirectory is idempotent if it exists)
        SDL_CreateDirectory(logs_dir.c_str());

        // Rolling rotation: keep at most MAX_LOG_FILES_c before opening new one
        enforceLogRotation(logs_dir);

        std::string full_path = logs_dir + currentLogFilename;
        logFile = fopen(full_path.c_str(), "w");
    }

    if (!logFile) {
        // Fallback: current working directory
        logFile = fopen(currentLogFilename, "w");

        if (!logFile) {
            // Last resort: application data path
            const char *appFilename = getApplicationFilename(currentLogFilename, false);
            logFile = fopen(appFilename, "w");
            // If this also fails, logging to file is simply unavailable;
            // messages still go to stdout/stderr via sdl3CustomLogger.
        }
    }

    // Log startup banner
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "=== Gigalomania Log Started ===");
    if (logFile) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Log file: %s", currentLogFilename);
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Version %d.%d.%d",
                majorVersion, minorVersion, patchVersion);

#ifdef _DEBUG
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running in Debug mode");
#else
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running in Release mode");
#endif

#if defined(_WIN32)
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Platform: Windows");
#elif defined(__ANDROID__)
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Platform: Android");
#elif defined(__linux__)
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Platform: Linux");
#elif defined(__APPLE__) && defined(__MACH__)
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Platform: macOS");
#else
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Platform: UNKNOWN");
#endif
}

// Cleanup function to properly close log files
void cleanupSDL3Logging() {
    if (logFile) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "=== Gigalomania Log Ended ===");
        fclose(logFile);
        logFile = nullptr;
    }

    if (currentLogFilename) {
        delete[] currentLogFilename;
        currentLogFilename = nullptr;
    }
}

// Set SDL log priority for SDL_LOG_CATEGORY_APPLICATION.
// Call after argument parsing to activate the user-requested level.
void setSDL3LogLevel(SDL_LogPriority priority) {
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, priority);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Log level set to %d", (int)priority);
}

// Parse a log-level string into the corresponding SDL_LogPriority.
// Accepted strings: verbose, debug, info, warn, error, critical.
// Returns SDL_LOG_PRIORITY_INFO on unrecognised input (with a warning).
SDL_LogPriority parseSDL3LogLevel(const char *str) {
    if (!str) return SDL_LOG_PRIORITY_INFO;

    if (strcmp(str, "verbose") == 0)  return SDL_LOG_PRIORITY_VERBOSE;
    if (strcmp(str, "debug") == 0)    return SDL_LOG_PRIORITY_DEBUG;
    if (strcmp(str, "info") == 0)     return SDL_LOG_PRIORITY_INFO;
    if (strcmp(str, "warn") == 0)     return SDL_LOG_PRIORITY_WARN;
    if (strcmp(str, "error") == 0)    return SDL_LOG_PRIORITY_ERROR;
    if (strcmp(str, "critical") == 0) return SDL_LOG_PRIORITY_CRITICAL;

    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Unknown log level '%s', defaulting to INFO", str);
    return SDL_LOG_PRIORITY_INFO;
}
