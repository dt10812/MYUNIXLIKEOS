#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"

extern void terminal_write(const char*);

int cmd_uname(int argc, char** argv) {
    bool show_all = false;
    bool show_sysname = false;
    bool show_nodename = false;
    bool show_release = false;
    bool show_version = false;
    bool show_machine = false;
    bool first = true;

    // Parse flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            show_all = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sysname") == 0) {
            show_sysname = true;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nodename") == 0) {
            show_nodename = true;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--release") == 0) {
            show_release = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            show_version = true;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--machine") == 0) {
            show_machine = true;
        }
    }

    // Default: show sysname only
    if (!show_all && !show_sysname && !show_nodename && 
        !show_release && !show_version && !show_machine) {
        show_sysname = true;
    }

    if (show_all) {
        show_sysname = true;
        show_nodename = true;
        show_release = true;
        show_version = true;
        show_machine = true;
    }

    // System name
    if (show_sysname) {
        if (!first) terminal_write(" ");
        terminal_write("MYUNIXLIKEOS");
        first = false;
    }

    // Node name (hostname)
    if (show_nodename) {
        if (!first) terminal_write(" ");
        terminal_write("dev-null");
        first = false;
    }

    // Release version
    if (show_release) {
        if (!first) terminal_write(" ");
        terminal_write("1.0.0");
        first = false;
    }

    // Version info
    if (show_version) {
        if (!first) terminal_write(" ");
        terminal_write("#1 SMP");
        first = false;
    }

    // Machine type
    if (show_machine) {
        if (!first) terminal_write(" ");
        terminal_write("i386");
        first = false;
    }

    terminal_write("\n");
    return 0;
}
