#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

int cmd_head(int argc, char** argv) {
    int num_lines = 10;
    int file_arg = 1;
    
    if (argc < 2) {
        printf("head: missing filename\n");
        printf("Usage: head [-n NUM] <file>\n");
        return 1;
    }
    
    /* Parse -n flag */
    if (argv[1][0] == '-' && argv[1][1] == 'n') {
        if (argc < 4) {
            printf("head: -n requires an argument\n");
            return 1;
        }
        
        num_lines = 0;
        for (int i = 0; argv[2][i]; i++) {
            if (argv[2][i] >= '0' && argv[2][i] <= '9') {
                num_lines = num_lines * 10 + (argv[2][i] - '0');
            }
        }
        file_arg = 3;
    }
    
    if (file_arg >= argc) {
        printf("head: missing filename\n");
        return 1;
    }
    
    vnode_t* node = vfs_lookup(argv[file_arg]);
    if (!node || !(node->flags & VFS_FILE)) {
        printf("head: cannot open '%s'\n", argv[file_arg]);
        return 1;
    }
    
    const char* content = node->content;
    if (!content) return 0;
    
    int lines_printed = 0;
    for (uint32_t i = 0; content[i] && lines_printed < num_lines; i++) {
        if (content[i] == '\n') {
            lines_printed++;
        }
        printf("%c", content[i]);
    }
    
    return 0;
}
