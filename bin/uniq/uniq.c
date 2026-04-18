#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

#define MAX_LINES 256
#define MAX_LINE_LEN 128

int cmd_uniq(int argc, char** argv) {
    if (argc < 2) {
        printf("uniq: missing filename\n");
        printf("Usage: uniq <file>\n");
        return 1;
    }
    
    vnode_t* node = vfs_lookup(argv[1]);
    if (!node || !(node->flags & VFS_FILE)) {
        printf("uniq: cannot open '%s'\n", argv[1]);
        return 1;
    }
    
    const char* content = node->content;
    if (!content) return 0;
    
    /* Extract lines */
    char lines[MAX_LINES][MAX_LINE_LEN];
    int num_lines = 0;
    int current_pos = 0;
    
    for (uint32_t i = 0; content[i] && num_lines < MAX_LINES; i++) {
        if (content[i] == '\n') {
            lines[num_lines][current_pos] = '\0';
            num_lines++;
            current_pos = 0;
        } else if (current_pos < MAX_LINE_LEN - 1) {
            lines[num_lines][current_pos++] = content[i];
        }
    }
    
    if (current_pos > 0) {
        lines[num_lines][current_pos] = '\0';
        num_lines++;
    }
    
    /* Print unique lines (remove consecutive duplicates) */
    if (num_lines > 0) {
        printf("%s\n", lines[0]);
        
        for (int i = 1; i < num_lines; i++) {
            if (strcmp(lines[i], lines[i - 1]) != 0) {
                printf("%s\n", lines[i]);
            }
        }
    }
    
    return 0;
}
