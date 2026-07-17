# Skred Virtual File System

The VFS under `parts/exp-vfs/` is compiled into the main SKRED API. It provides
disk, file-backed ZIP, and memory-backed ZIP mounts behind one file/directory
interface. Skode loaders and filesystem commands use it, and the public
functions are installed through `include/skred/skred_vfs.h`.

The implementation maintains a VFS-relative current working directory. Paths
are lexically resolved before disk or ZIP lookup; `.` and `..` are collapsed
without allowing ZIP navigation above the mounted root. In disk mode, an
absolute path selects a real directory or file. Prefix a loader path with
`file:` when a ZIP is mounted and the command must bypass the mount.

## Build and Dependencies

The main CMake build compiles:

- `skred_vfs.c`
- `miniz.c`
- `miniz_tdef.c`
- `miniz_tinfl.c`
- `miniz_zip.c`

The separate Makefile and test program in this directory are useful for
isolated VFS development. The older top-level `vfs/` directory is a standalone
prototype and is not the implementation linked into SKRED.

## Mount Lifecycle

- `skred_vfs_mount(path)` mounts a disk directory or a file-backed `.zip`.
- `skred_vfs_mount_zip_memory(data,size,label)` copies and mounts an in-memory
  ZIP, which is used by browser uploads.
- `skred_vfs_unmount()` returns to disk mode rooted at the real current
  directory.
- `skred_vfs_shutdown()` releases archive and memory state.
- `skred_vfs_mode()`, `skred_vfs_root()`, and `skred_vfs_status()` report the
  active mount.
- `skred_chdir()` and `skred_getcwd()` manage the VFS-relative working
  directory.

`skred_vfs_init()` is a compatibility alias for `skred_vfs_mount()`.

## File and Directory API

The stdio-like API is:

- `skred_fopen()`, `skred_fread()`, `skred_fwrite()`
- `skred_fseek()`, `skred_ftell()`, `skred_fclose()`
- `skred_opendir()`, `skred_readdir()`, `skred_closedir()`

Disk mode delegates to ordinary stdio and directory calls. ZIP reads extract a
member to an owned memory buffer. ZIP writes buffer a member and add it when
the handle closes; archive finalization occurs during unmount/shutdown.

Whole-file helpers are:

- `skred_vfs_read_file()` for the active VFS
- `skred_vfs_read_real_file()` to bypass the active mount
- `skred_vfs_free_file()` to release returned data

## Skode Surface

The main user-facing commands are:

- `[bundle.zip] %z` — mount a disk ZIP
- `%zu` — unmount
- `%pwd`, `%cd`, `%ls`, `%cat` — inspect and navigate the active view

File, wave, Skode, and Ksynth loaders search the mounted VFS first, then the
real current directory and their type-specific fallback directories. See
`SKODE_USER_COMMAND_REFERENCE.md` for command syntax and search behavior.
