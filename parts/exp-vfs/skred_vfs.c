#include "skred_vfs.h"
#include "miniz.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

typedef enum {
    VFS_MODE_DISK,
    VFS_MODE_ZIP
} VfsMode;

typedef struct {
    VfsMode mode;
    char base_path[256];
    mz_zip_archive archive;
    char cwd[512]; 
} VfsState;

static VfsState g_vfs = { .mode = VFS_MODE_DISK };

struct SkredFile {
    VfsMode mode;
    char resolved_path[512];
    bool is_writing;
    union {
        FILE *disk_file;
        struct {
            unsigned char *buffer;
            size_t size;
            size_t capacity;
            size_t cursor;
        } mem;
    } handle;
};

struct SkredDir {
    VfsMode mode;
    union {
        DIR *disk_dir;
        struct {
            unsigned int current_index;
            unsigned int total_files;
            char filter_prefix[256];
            size_t prefix_len;
        } zip;
    } handle;
    SkredDirent entry;
};

/* --- Internal Helpers --- */

static bool vfs_is_zip(const char *path) {
    const char *ext = strrchr(path, '.');
    return (ext && strcmp(ext, ".zip") == 0);
}

// Lexically resolves . and .. without touching the physical filesystem
static void vfs_resolve_path(const char *cwd, const char *input, char *output, size_t out_len) {
    char temp[1024];
    
    // Absolute paths (relative to VFS root) vs Relative paths
    if (input[0] == '/') {
        strncpy(temp, input, sizeof(temp));
    } else {
        if (strlen(cwd) > 0) {
            snprintf(temp, sizeof(temp), "%s/%s", cwd, input);
        } else {
            strncpy(temp, input, sizeof(temp));
        }
    }

    char *stack[64];
    int top = 0;
    
    char *token = strtok(temp, "/");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            // No-op
        } else if (strcmp(token, "..") == 0) {
            if (top > 0) top--;
        } else if (strlen(token) > 0) {
            stack[top++] = token;
        }
        token = strtok(NULL, "/");
    }

    output[0] = '\0';
    size_t current_len = 0;
    for (int i = 0; i < top; i++) {
        size_t token_len = strlen(stack[i]);
        if (current_len + token_len + 2 > out_len) break; // Bounds safety
        strcat(output, stack[i]);
        current_len += token_len;
        if (i < top - 1) {
            strcat(output, "/");
            current_len++;
        }
    }
}

static bool zip_dir_exists(const char* prefix) {
    if (strlen(prefix) == 0) return true; // Root always exists
    
    char search_prefix[256];
    snprintf(search_prefix, sizeof(search_prefix), "%s/", prefix);
    
    int total = mz_zip_reader_get_num_files(&g_vfs.archive);
    for(int i = 0; i < total; i++) {
        mz_zip_file_stat stat;
        if (mz_zip_reader_file_stat(&g_vfs.archive, i, &stat)) {
            if (strncmp(stat.m_filename, search_prefix, strlen(search_prefix)) == 0) {
                return true;
            }
        }
    }
    return false;
}

/* --- Lifecycle --- */

bool skred_vfs_init(const char *path) {
    strncpy(g_vfs.base_path, path, sizeof(g_vfs.base_path) - 1);
    memset(&g_vfs.archive, 0, sizeof(g_vfs.archive));
    g_vfs.cwd[0] = '\0'; // Start at VFS root

    if (vfs_is_zip(path)) {
        if (!mz_zip_reader_init_file(&g_vfs.archive, path, 0)) {
            if (!mz_zip_writer_init_file(&g_vfs.archive, path, 0)) {
                return false;
            }
        }
        g_vfs.mode = VFS_MODE_ZIP;
    } else {
        g_vfs.mode = VFS_MODE_DISK;
    }
    return true;
}

void skred_vfs_shutdown(void) {
    if (g_vfs.mode == VFS_MODE_ZIP) {
        if (g_vfs.archive.m_zip_mode == MZ_ZIP_MODE_WRITING) {
            mz_zip_writer_finalize_archive(&g_vfs.archive);
            mz_zip_writer_end(&g_vfs.archive);
        } else {
            mz_zip_reader_end(&g_vfs.archive);
        }
    }
}

/* --- Environment State --- */

bool skred_chdir(const char *path) {
    char target_path[512];
    vfs_resolve_path(g_vfs.cwd, path, target_path, sizeof(target_path));

    if (strlen(target_path) == 0 || strcmp(target_path, "/") == 0) {
        g_vfs.cwd[0] = '\0';
        return true;
    }

    if (g_vfs.mode == VFS_MODE_DISK) {
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", g_vfs.base_path, target_path);
        DIR *d = opendir(full);
        if (d) {
            closedir(d);
            strncpy(g_vfs.cwd, target_path, sizeof(g_vfs.cwd) - 1);
            return true;
        }
        return false;
    } else {
        if (zip_dir_exists(target_path)) {
            strncpy(g_vfs.cwd, target_path, sizeof(g_vfs.cwd) - 1);
            return true;
        }
        return false;
    }
}

const char* skred_getcwd(void) {
    return (strlen(g_vfs.cwd) == 0) ? "/" : g_vfs.cwd;
}

/* --- File I/O --- */

SkredFile* skred_fopen(const char *filepath, const char *mode) {
    SkredFile *file = calloc(1, sizeof(SkredFile));
    if (!file) return NULL;

    vfs_resolve_path(g_vfs.cwd, filepath, file->resolved_path, sizeof(file->resolved_path));
    file->mode = g_vfs.mode;
    file->is_writing = (strchr(mode, 'w') || strchr(mode, 'a'));

    if (file->mode == VFS_MODE_DISK) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_vfs.base_path, file->resolved_path);
        file->handle.disk_file = fopen(full_path, mode);
        if (!file->handle.disk_file) {
            free(file);
            return NULL;
        }
    } 
    else {
        if (file->is_writing) {
            file->handle.mem.capacity = 1024;
            file->handle.mem.buffer = malloc(file->handle.mem.capacity);
            file->handle.mem.size = 0;
            file->handle.mem.cursor = 0;
            
            if (g_vfs.archive.m_zip_mode != MZ_ZIP_MODE_WRITING) {
                mz_zip_writer_init_from_reader(&g_vfs.archive, g_vfs.base_path);
            }
        } else {
            int idx = mz_zip_reader_locate_file(&g_vfs.archive, file->resolved_path, NULL, 0);
            if (idx < 0) { free(file); return NULL; }

            size_t size;
            file->handle.mem.buffer = mz_zip_reader_extract_to_heap(&g_vfs.archive, idx, &size, 0);
            file->handle.mem.size = size;
            file->handle.mem.cursor = 0;
        }
    }
    return file;
}

size_t skred_fread(void *ptr, size_t size, size_t count, SkredFile *stream) {
    if (stream->mode == VFS_MODE_DISK) {
        return fread(ptr, size, count, stream->handle.disk_file);
    }
    size_t requested = size * count;
    size_t available = stream->handle.mem.size - stream->handle.mem.cursor;
    size_t to_read = (requested > available) ? available : requested;
    if (to_read == 0) return 0;
    
    memcpy(ptr, stream->handle.mem.buffer + stream->handle.mem.cursor, to_read);
    stream->handle.mem.cursor += to_read;
    return to_read / size;
}

size_t skred_fwrite(const void *ptr, size_t size, size_t count, SkredFile *stream) {
    if (stream->mode == VFS_MODE_DISK) {
        return fwrite(ptr, size, count, stream->handle.disk_file);
    }
    
    size_t requested = size * count;
    if (stream->handle.mem.cursor + requested > stream->handle.mem.capacity) {
        stream->handle.mem.capacity = (stream->handle.mem.cursor + requested) * 2;
        stream->handle.mem.buffer = realloc(stream->handle.mem.buffer, stream->handle.mem.capacity);
    }

    memcpy(stream->handle.mem.buffer + stream->handle.mem.cursor, ptr, requested);
    stream->handle.mem.cursor += requested;
    if (stream->handle.mem.cursor > stream->handle.mem.size) {
        stream->handle.mem.size = stream->handle.mem.cursor;
    }
    return count;
}

int skred_fseek(SkredFile *stream, long offset, int origin) {
    if (stream->mode == VFS_MODE_DISK) {
        return fseek(stream->handle.disk_file, offset, origin);
    }
    long next = 0;
    if (origin == SEEK_SET) next = offset;
    else if (origin == SEEK_CUR) next = (long)stream->handle.mem.cursor + offset;
    else if (origin == SEEK_END) next = (long)stream->handle.mem.size + offset;
    
    if (next < 0 || next > (long)stream->handle.mem.size) return -1;
    stream->handle.mem.cursor = (size_t)next;
    return 0;
}

long skred_ftell(SkredFile *stream) {
    return (stream->mode == VFS_MODE_DISK) ? ftell(stream->handle.disk_file) : (long)stream->handle.mem.cursor;
}

int skred_fclose(SkredFile *stream) {
    if (stream->mode == VFS_MODE_DISK) {
        fclose(stream->handle.disk_file);
    } else {
        if (stream->is_writing) {
            mz_zip_writer_add_mem(&g_vfs.archive, stream->resolved_path, stream->handle.mem.buffer, stream->handle.mem.size, MZ_DEFAULT_COMPRESSION);
        }
        free(stream->handle.mem.buffer);
    }
    free(stream);
    return 0;
}

/* --- Directory Iteration --- */

SkredDir* skred_opendir(const char *dirpath) {
    char target_path[512];
    vfs_resolve_path(g_vfs.cwd, dirpath, target_path, sizeof(target_path));

    SkredDir *dir = calloc(1, sizeof(SkredDir));
    dir->mode = g_vfs.mode;

    if (dir->mode == VFS_MODE_DISK) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", g_vfs.base_path, target_path);
        dir->handle.disk_dir = opendir(path);
        if (!dir->handle.disk_dir) { free(dir); return NULL; }
    } else {
        dir->handle.zip.total_files = mz_zip_reader_get_num_files(&g_vfs.archive);
        if (strlen(target_path) > 0) {
            snprintf(dir->handle.zip.filter_prefix, 256, "%s/", target_path);
            dir->handle.zip.prefix_len = strlen(dir->handle.zip.filter_prefix);
        }
    }
    return dir;
}

SkredDirent* skred_readdir(SkredDir *dirp) {
    if (dirp->mode == VFS_MODE_DISK) {
        struct dirent *d = readdir(dirp->handle.disk_dir);
        if (!d) return NULL;
        strncpy(dirp->entry.d_name, d->d_name, 255);
        dirp->entry.is_directory = (d->d_type == DT_DIR);
        return &dirp->entry;
    }

    while (dirp->handle.zip.current_index < dirp->handle.zip.total_files) {
        mz_zip_file_stat stat;
        if (!mz_zip_reader_file_stat(&g_vfs.archive, dirp->handle.zip.current_index++, &stat)) {
            continue;
        }
        
        if (dirp->handle.zip.prefix_len > 0) {
            if (strncmp(stat.m_filename, dirp->handle.zip.filter_prefix, dirp->handle.zip.prefix_len) != 0) continue;
        }

        const char *rel = stat.m_filename + dirp->handle.zip.prefix_len;
        if (!*rel) continue;

        const char *slash = strchr(rel, '/');
        if (slash) {
            if (*(slash + 1) != '\0') continue; // Deeper nested file
            dirp->entry.is_directory = true;
            size_t len = slash - rel;
            strncpy(dirp->entry.d_name, rel, len);
            dirp->entry.d_name[len] = '\0';
        } else {
            dirp->entry.is_directory = stat.m_is_directory;
            strncpy(dirp->entry.d_name, rel, 255);
        }
        return &dirp->entry;
    }
    return NULL;
}

int skred_closedir(SkredDir *dirp) {
    if (dirp->mode == VFS_MODE_DISK) closedir(dirp->handle.disk_dir);
    free(dirp);
    return 0;
}
