#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

#define MAX_LINES 256
#define MAX_LINE_LEN 128

int cmd_sort(int argc, char** argv) {
    if (argc < 2) {
        printf("sort: missing filename\n");
        printf("Usage: sort <file>\n");
        return 1;
    }
    
    vnode_t* node = vfs_lookup(argv[1]);
    if (!node || !(node->flags & VFS_FILE)) {
        printf("sort: cannot open '%s'\n", argv[1]);
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
    
    /* Bubble sort */
    for (int i = 0; i < num_lines - 1; i++) {
        for (int j = 0; j < num_lines - 1 - i; j++) {
            if (strcmp(lines[j], lines[j + 1]) > 0) {
                char tmp[MAX_LINE_LEN];
                strcpy(tmp, lines[j]);
                strcpy(lines[j], lines[j + 1]);
                strcpy(lines[j + 1], tmp);
            }
        }
    }
    
    /* Print sorted lines */
    for (int i = 0; i < num_lines; i++) {
        printf("%s\n", lines[i]);
    }
    
    return 0;
}
