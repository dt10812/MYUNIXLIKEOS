#include "commands.h"
#include "vfs.h"
#include "stdio.h"
#include "string.h"

int cmd_wc(int argc, char** argv) {
    if (argc < 2) {
        printf("wc: missing filename\n");
        printf("Usage: wc [-lwc] <file>\n");
        return 1;
    }
    
    bool count_lines = true;
    bool count_words = true;
    bool count_chars = true;
    int file_arg = 1;
    
    /* Parse flags */
    if (argv[1][0] == '-') {
        count_lines = false;
        count_words = false;
        count_chars = false;
        
        if (strchr(argv[1], 'l')) count_lines = true;
        if (strchr(argv[1], 'w')) count_words = true;
        if (strchr(argv[1], 'c')) count_chars = true;
        
        file_arg = 2;
    }
    
    if (file_arg >= argc) {
        printf("wc: missing filename\n");
        return 1;
    }
    
    vnode_t* node = vfs_lookup(argv[file_arg]);
    if (!node || !(node->flags & VFS_FILE)) {
        printf("wc: cannot open '%s'\n", argv[file_arg]);
        return 1;
    }
    
    const char* content = node->content;
    if (!content) {
        printf("0 0 0\n");
        return 0;
    }
    
    uint32_t lines = 0;
    uint32_t words = 0;
    uint32_t chars = 0;
    bool in_word = false;
    
    for (uint32_t i = 0; content[i]; i++) {
        char c = content[i];
        chars++;
        
        if (c == '\n') {
            lines++;
            in_word = false;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            in_word = false;
        } else {
            if (!in_word) {
                words++;
                in_word = true;
            }
        }
    }
    
    if (in_word) lines++;  /* Count last line if no trailing newline */
    
    if (count_lines) printf("%u ", lines);
    if (count_words) printf("%u ", words);
    if (count_chars) printf("%u", chars);
    printf("\n");
    
    return 0;
}
