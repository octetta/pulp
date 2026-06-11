# Skred Virtual File System (VFS)

> Experimental standalone copy. This API is not currently wired into the main
> SKRED CMake build or the Skode file commands.

The Skred VFS is a lightweight abstraction layer designed to unify file system interactions within the skred audio engine. It allows the engine to mount either a standard physical directory or a flat `.zip` archive, providing a unified API to read files, write files, and iterate directory structures.

Crucially, it implements an internal **Current Working Directory (CWD)**. This stateful navigation allows commands like `cd` and `ls` to function transparently across both physical hierarchies and stateless zip archives.

## Dependencies & Integration

This module requires **miniz**, a single-file ZIP/Deflate library.

1. Ensure `miniz.c` and `miniz.h` are present in `vfs/`.
2. Run `make` in `vfs/` to build the standalone `vfs_test`.
3. To embed it elsewhere, compile `miniz.c` and `skred_vfs.c` and include
   `miniz.h` and `skred_vfs.h`.

## Architecture Notes

* **Lexical Path Resolution:** All paths passed to `skred_fopen`, `skred_opendir`, and `skred_chdir` are first lexically resolved against the VFS CWD. Relative traversals (`../`) are collapsed in memory before interacting with disk or zip data.
* **Write Buffering in ZIP Mode:** Opening a file in ZIP mode with `"wb"` allocates an expanding heap buffer. All `skred_fwrite` calls write to this buffer. The actual Deflate compression and insertion into the central ZIP directory occurs *only* when `skred_fclose` is called.

## API Reference

### Initialization & State

* `bool skred_vfs_init(const char *path)`: Mounts a target. If `path` ends in `.zip`, the engine enters ZIP mode. Otherwise, it operates in DISK mode rooted at `path`.
* `void skred_vfs_shutdown(void)`: Closes open file handles and finalizes ZIP archives if writes were performed.
* `bool skred_chdir(const char *path)`: Changes the internal working directory. Resolves lexically and verifies directory existence.
* `const char* skred_getcwd(void)`: Returns the current absolute VFS path.

### Standard I/O Replicas

* `SkredFile* skred_fopen(const char *filepath, const char *mode)`
* `size_t skred_fread(void *ptr, size_t size, size_t count, SkredFile *stream)`
* `size_t skred_fwrite(const void *ptr, size_t size, size_t count, SkredFile *stream)`
* `int skred_fseek(SkredFile *stream, long offset, int origin)`
* `long skred_ftell(SkredFile *stream)`
* `int skred_fclose(SkredFile *stream)`

### Directory Iteration

* `SkredDir* skred_opendir(const char *dirpath)`
* `SkredDirent* skred_readdir(SkredDir *dirp)`
* `int skred_closedir(SkredDir *dirp)`
