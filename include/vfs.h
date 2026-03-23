#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02
#define VFS_CHAR_DEVICE 0x03

struct vnode;

typedef struct vfs_entry {
    char name[128];
    uint64_t inode;
    uint32_t type;
    struct vnode* node;
} vfs_entry_t;

typedef struct vnode {
    char name[32];
    uint32_t flags; // VFS_FILE or VFS_DIRECTORY
    struct vnode* parent;
    struct vnode* children[16];
    uint32_t child_count;
    char* content; // for files
} vnode_t;

// Global VFS Root
extern vnode_t* vfs_root;

// Kernel API for File Operations
void vfs_init();
int k_mkdir(const char* path);
int k_touch(const char* path);
vnode_t* vfs_lookup(const char* path);

#endif