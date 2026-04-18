#include "commands.h"
#include "stdio.h"
#include "string.h"

extern void terminal_write(const char*);

int cmd_man(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("man: Show manual pages for commands\n");
        terminal_write("Usage: man <command>\n");
        return 0;
    }
    
    const char* cmd = argv[1];
    
    if (!strcmp(cmd, "ls")) {
        printf("LS(1)\n\n");
        printf("NAME\n");
        printf("  ls - list directory contents\n\n");
        printf("SYNOPSIS\n");
        printf("  ls [-al]\n\n");
        printf("DESCRIPTION\n");
        printf("  List files and directories in current directory.\n");
        printf("  -a    Show hidden files (starting with .)\n");
        printf("  -l    Long format with file sizes and types\n");
    } else if (!strcmp(cmd, "cat")) {
        printf("CAT(1)\n\n");
        printf("NAME\n");
        printf("  cat - concatenate and display files\n\n");
        printf("SYNOPSIS\n");
        printf("  cat <file>\n\n");
        printf("DESCRIPTION\n");
        printf("  Display contents of a file.\n");
    } else if (!strcmp(cmd, "wc")) {
        printf("WC(1)\n\n");
        printf("NAME\n");
        printf("  wc - count lines, words, and characters\n\n");
        printf("SYNOPSIS\n");
        printf("  wc [-lwc] <file>\n\n");
        printf("DESCRIPTION\n");
        printf("  Count lines, words, or characters in a file.\n");
        printf("  -l    Count lines only\n");
        printf("  -w    Count words only\n");
        printf("  -c    Count characters only\n");
    } else if (!strcmp(cmd, "head")) {
        printf("HEAD(1)\n\n");
        printf("NAME\n");
        printf("  head - display first lines of a file\n\n");
        printf("SYNOPSIS\n");
        printf("  head [-n NUM] <file>\n\n");
        printf("DESCRIPTION\n");
        printf("  Display the first NUM lines of a file (default 10).\n");
    } else if (!strcmp(cmd, "tail")) {
        printf("TAIL(1)\n\n");
        printf("NAME\n");
        printf("  tail - display last lines of a file\n\n");
        printf("SYNOPSIS\n");
        printf("  tail [-n NUM] <file>\n\n");
        printf("DESCRIPTION\n");
        printf("  Display the last NUM lines of a file (default 10).\n");
    } else if (!strcmp(cmd, "sort")) {
        printf("SORT(1)\n\n");
        printf("NAME\n");
        printf("  sort - sort lines of a file\n\n");
        printf("SYNOPSIS\n");
        printf("  sort <file>\n\n");
        printf("DESCRIPTION\n");
        printf("  Sort lines of a file alphabetically.\n");
    } else if (!strcmp(cmd, "uniq")) {
        printf("UNIQ(1)\n\n");
        printf("NAME\n");
        printf("  uniq - remove duplicate lines\n\n");
        printf("SYNOPSIS\n");
        printf("  uniq <file>\n\n");
        printf("DESCRIPTION\n");
        printf("  Remove consecutive duplicate lines from a file.\n");
    } else if (!strcmp(cmd, "whoami")) {
        printf("WHOAMI(1)\n\n");
        printf("NAME\n");
        printf("  whoami - display current user\n\n");
        printf("SYNOPSIS\n");
        printf("  whoami\n\n");
        printf("DESCRIPTION\n");
        printf("  Display the login name of the current user.\n");
    } else {
        printf("man: No manual entry for '%s'\n", cmd);
        return 1;
    }
    
    return 0;
}
