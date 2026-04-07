#include <stdint.h>
#include <time.h>

int64_t ts_diff_ns(const struct timespec *a, const struct timespec *b) {
  return ((int64_t)b->tv_sec  - a->tv_sec)  * 1000000000LL +
    ((int64_t)b->tv_nsec - a->tv_nsec);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"


/**
 * Retrieves the absolute path of the running executable.
 * Returns 0 on success, -1 on failure.
 */
int get_executable_path(char* buffer, size_t size) {
#if defined(_WIN32)
    DWORD result = GetModuleFileNameA(NULL, buffer, (DWORD)size);
    if (result == 0 || result == size) return -1;
    return 0;
#elif defined(__APPLE__)
    uint32_t bufsize = (uint32_t)size;
    if (_NSGetExecutablePath(buffer, &bufsize) == 0) return 0;
    return -1;
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return 0;
    }
    return -1;
#endif
    return -1;
}

/**
 * Extracts the directory from a full file path.
 */
void get_directory(const char* full_path, char* dir_out) {
    const char* last_sep = strrchr(full_path, PATH_SEP);
    if (last_sep) {
        size_t dir_len = (size_t)(last_sep - full_path);
        strncpy(dir_out, full_path, dir_len);
        dir_out[dir_len] = '\0';
    } else {
        strcpy(dir_out, ".");
    }
}

/**
 * Joins a base directory with a relative subpath.
 * Correctly handles the separator.
 */
int join_path(char* buffer, size_t size, const char* base, const char* subpath) {
    int res = snprintf(buffer, size, "%s%c%s", base, PATH_SEP, subpath);
    // snprintf returns the number of chars that WOULD have been written
    return (res >= 0 && (size_t)res < size) ? 0 : -1;
}

/*
int main() {
    char exe_path[MAX_PATH_LEN];
    char root_dir[MAX_PATH_LEN];
    char config_path[MAX_PATH_LEN];
    char asset_path[MAX_PATH_LEN];

    // 1. Get the binary path
    if (get_executable_path(exe_path, sizeof(exe_path)) != 0) {
        fprintf(stderr, "Error: Could not determine executable path.\n");
        return 1;
    }

    // 2. Get the directory containing the binary
    get_directory(exe_path, root_dir);

    // 3. Case A: Load a file in the SAME folder (e.g., config.ace)
    if (join_path(config_path, sizeof(config_path), root_dir, "config.ace") == 0) {
        printf("Config Path: %s\n", config_path);
    }

    // 4. Case B: Load a file in a SUBDIRECTORY (e.g., assets/kick.wav)
    // Note: Use the platform-appropriate separator in the string or join again
    char assets_dir[MAX_PATH_LEN];
    join_path(assets_dir, sizeof(assets_dir), root_dir, "assets");
    join_path(asset_path, sizeof(asset_path), assets_dir, "kick.wav");

    printf("Asset Path:  %s\n", asset_path);

    // * Experienced Engineer Tip:
    // If you are loading many assets, store 'root_dir' globally once 
    // at startup so you don't have to recalculate it every time.

    return 0;
}
*/
