#ifndef SKRED_VFS_H
#define SKRED_VFS_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

/* --- Types --- */

typedef struct SkredFile SkredFile;
typedef struct SkredDir SkredDir;

typedef struct {
    char d_name[256];
    bool is_directory;
} SkredDirent;

/* --- Lifecycle --- */

// Initialize the VFS. Path can be a directory or a .zip file.
bool skred_vfs_init(const char *path);
void skred_vfs_shutdown(void);

/* --- Environment State --- */

bool        skred_chdir(const char *path);
const char* skred_getcwd(void);

/* --- File I/O --- */

SkredFile* skred_fopen(const char *filepath, const char *mode);
size_t     skred_fread(void *ptr, size_t size, size_t count, SkredFile *stream);
size_t     skred_fwrite(const void *ptr, size_t size, size_t count, SkredFile *stream);
long       skred_ftell(SkredFile *stream);
int        skred_fseek(SkredFile *stream, long offset, int origin);
int        skred_fclose(SkredFile *stream);

/* --- Directory Iteration --- */

SkredDir*    skred_opendir(const char *dirpath);
SkredDirent* skred_readdir(SkredDir *dirp);
int          skred_closedir(SkredDir *dirp);

#endif // SKRED_VFS_H
