#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

int cmd_tail(int argc, char** argv) {
    int num_lines = 10;
    int file_arg = 1;
    
    if (argc < 2) {
        printf("tail: missing filename\n");
        printf("Usage: tail [-n NUM] <file>\n");
        return 1;
    }
    
    /* Parse -n flag */
    if (argv[1][0] == '-' && argv[1][1] == 'n') {
        if (argc < 4) {
            printf("tail: -n requires an argument\n");
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
        printf("tail: missing filename\n");
        return 1;
    }
    
    vnode_t* node = vfs_lookup(argv[file_arg]);
    if (!node || !(node->flags & VFS_FILE)) {
        printf("tail: cannot open '%s'\n", argv[file_arg]);
        return 1;
    }
    
    const char* content = node->content;
    if (!content) return 0;
    
    /* Count total lines */
    int total_lines = 1;
    for (uint32_t i = 0; content[i]; i++) {
        if (content[i] == '\n') total_lines++;
    }
    
    /* Calculate starting line */
    int start_line = total_lines - num_lines;
    if (start_line < 1) start_line = 1;
    
    /* Print from start line */
    int current_line = 1;
    for (uint32_t i = 0; content[i]; i++) {
        if (current_line >= start_line) {
            printf("%c", content[i]);
        }
        
        if (content[i] == '\n') {
            current_line++;
        }
    }
    
    return 0;
}
