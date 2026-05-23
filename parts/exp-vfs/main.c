#include "skred_vfs.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("--- Initializing Skred VFS ---\n");
    
    // Mount a ZIP file. Because it doesn't exist yet, the VFS will create it.
    if (!skred_vfs_init("assets_production.zip")) {
        fprintf(stderr, "Failed to initialize VFS.\n");
        return 1;
    }

    // 1. Write a file deep into a directory structure inside the ZIP
    printf("Writing 'kick.wav' to /samples/drums/ ...\n");
    SkredFile *f = skred_fopen("samples/drums/kick.wav", "wb");
    if (f) {
        const char *dummy_data = "RIFF....WAVE"; // Dummy header
        skred_fwrite(dummy_data, 1, strlen(dummy_data), f);
        skred_fclose(f); // Commits memory to the ZIP archive
    }

    // 2. Write another file
    SkredFile *f2 = skred_fopen("samples/drums/snare.wav", "wb");
    if (f2) {
        skred_fwrite("snare_data", 1, 10, f2);
        skred_fclose(f2);
    }

    // 3. Emulate a 'cd' command
    printf("Changing VFS directory to 'samples/drums' ...\n");
    if (skred_chdir("samples/drums")) {
        printf("Current Working Directory: %s\n", skred_getcwd());

        // 4. Emulate an 'ls' command on the current directory
        printf("Directory listing:\n");
        SkredDir *dir = skred_opendir(".");
        if (dir) {
            SkredDirent *entry;
            while ((entry = skred_readdir(dir)) != NULL) {
                printf("  %s %s\n", entry->is_directory ? "[DIR] " : "[FILE]", entry->d_name);
            }
            skred_closedir(dir);
        }
    } else {
        printf("Failed to change directory.\n");
    }

    skred_vfs_shutdown();
    printf("--- Shutdown Complete ---\n");
    return 0;
}
