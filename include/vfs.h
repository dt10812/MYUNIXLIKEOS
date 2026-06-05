#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
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
    uint32_t flags;
    struct vnode* parent;
    struct vnode* children[16];
    uint32_t child_count;
    uint32_t size;
    char* content;
} vnode_t;

extern vnode_t* vfs_root;
extern vnode_t* current_dir;

void fs_init(void);

int k_mkdir(const char* path);
int k_touch(const char* path);
vnode_t* vfs_lookup(const char* path);
int k_install(const char* name, const uint8_t* data, uint32_t size);
int k_unlink(const char* path);
int k_exec(const char *path, const char **argv);

/* Enhanced VFS operations */
int vfs_stat(const char* path, vnode_t** out);
int vfs_isdir(const char* path);
int vfs_isfile(const char* path);
uint32_t vfs_filesize(const char* path);
int vfs_readdir(const char* path, vnode_t*** entries, uint32_t* count);
const char* vfs_read_file(const char* path, uint32_t* size_out);
int vfs_write_file(const char* path, const char* content, uint32_t size);
int vfs_append_file(const char* path, const char* content, uint32_t size);
vnode_t* vfs_find_in_path(vnode_t* dir, const char* name);
uint32_t vfs_child_count(const char* path);
vnode_t* vfs_get_child(const char* path, uint32_t index);

#endif