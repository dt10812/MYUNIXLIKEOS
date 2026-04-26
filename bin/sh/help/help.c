#include "commands.h"
#include "stdio.h"
#include "string.h"

extern void terminal_write(const char*);

int cmd_help(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("Available commands:\n");
        terminal_write("FILE: cat, cd, clear, cp, grep, head, ls, mkdir, mv, nano, rm, sort, tail, touch, uniq, wc\n");
        terminal_write("UTIL: alias, calc, date, dmesg, echo, man, password, pwd, color, memory, free, top, vmstat, uptime, which, whoami\n");
        terminal_write("DEV: gcc\n");
        terminal_write("GAMES: snake, vi\n");
        terminal_write("SYSTEM: help, shutdown, reboot, sysinfo\n");
        terminal_write("Usage: help [command]\n");
        return 0;
    }

    char* cmd = argv[1];

    if (!strcmp(cmd, "ls")) {
        printf("ls [-a]: List files. -a shows . and ..\n");
    } else if (!strcmp(cmd, "cd")) {
        printf("cd [dir]: Change directory.\n");
    } else if (!strcmp(cmd, "cat")) {
        printf("cat [file]: Read file content.\n");
    } else if (!strcmp(cmd, "pwd")) {
        printf("pwd: Print working directory.\n");
    } else if (!strcmp(cmd, "date")) {
        printf("date: Display current date and time.\n");
    } else if (!strcmp(cmd, "cp")) {
        printf("cp [source] [dest]: Copy file from source to destination.\n");
    } else if (!strcmp(cmd, "mv")) {
        printf("mv [source] [dest]: Move/rename file.\n");
    } else if (!strcmp(cmd, "rm")) {
        printf("rm [file]: Remove (delete) a file.\n");
    } else if (!strcmp(cmd, "nano")) {
        printf("nano [file]: Edit file with enhanced nano editor.\n");
        printf("  Ctrl+O Save  Ctrl+X Exit  Ctrl+G Help\n");
        printf("  Ctrl+A Home  Ctrl+E End   Ctrl+W Search\n");
        printf("  Ctrl+P Up    Ctrl+N Down  Ctrl+L GoToLine\n");
    } else if (!strcmp(cmd, "fs")) {
        printf("fs [type]: Supports ntfs, exfat, fat32, ext4, apfs.\n");
    } else if (!strcmp(cmd, "echo")) {
        printf("echo [text]: Print text to terminal.\n");
    } else if (!strcmp(cmd, "color")) {
        printf("color <foreground> [background]: Change text foreground and optional background.\n");
        printf("Colors are names or 0xN, and 0xNN can set both at once.\n");
        printf("Supported names: black, blue, green, cyan, red, magenta, brown, lightgray, darkgray, lightblue, lightgreen, lightcyan, lightred, lightmagenta, yellow, white.\n");
    } else if (!strcmp(cmd, "memory")) {
        printf("memory: Show OS memory usage for physical page allocation.\n");
        printf("Usage: memory\n");
    } else if (!strcmp(cmd, "free")) {
        printf("free: Display amount of free and used memory in the system.\n");
        printf("Usage: free\n");
    } else if (!strcmp(cmd, "top")) {
        printf("top: Display Linux-like tasks and memory usage.\n");
        printf("Usage: top\n");
    } else if (!strcmp(cmd, "vmstat")) {
        printf("vmstat: Report virtual memory statistics.\n");
        printf("Usage: vmstat\n");
    } else if (!strcmp(cmd, "snake")) {
        printf("snake: Start a simple snake game using W,A,S,D to move.\n");
        printf("Use: snake          # start game\n");
        printf("     snake color <snake|food|border|board|text> <color>\n");
        printf("     snake reset     # restore default game colors\n");
    } else if (!strcmp(cmd, "grep")) {
        printf("grep [pattern] [file]: Search in file.\n");
    } else if (!strcmp(cmd, "mkdir")) {
        printf("mkdir [name]: Create directory.\n");
    } else if (!strcmp(cmd, "touch")) {
        printf("touch [file]: Create empty file.\n");
    } else if (!strcmp(cmd, "shutdown")) {
        printf("shutdown: Shut down the system gracefully.\n");
    } else if (!strcmp(cmd, "reboot")) {
        printf("reboot: Reboot the system.\n");
    } else if (!strcmp(cmd, "calc")) {
        printf("calc [expression]: Simple calculator supporting +, -, *, /, ()\n");
        printf("Examples:\n");
        printf("  calc \"2+3\"\n");
        printf("  calc \"10-5\"\n");
        printf("  calc \"3*4\"\n");
        printf("  calc \"20/4\"\n");
        printf("  calc \"(2+3)*4\"\n");
    } else if (!strcmp(cmd, "man")) {
        printf("man [command]: Display manual pages for commands.\n");
        printf("Usage: man ls, man cat, man wc, etc.\n");
    } else if (!strcmp(cmd, "wc")) {
        printf("wc [-lwc] [file]: Count lines, words, and characters.\n");
        printf("  -l    Count lines only\n");
        printf("  -w    Count words only\n");
        printf("  -c    Count characters only\n");
    } else if (!strcmp(cmd, "head")) {
        printf("head [-n NUM] [file]: Display first NUM lines (default 10).\n");
        printf("Usage: head -n 5 file.txt\n");
    } else if (!strcmp(cmd, "tail")) {
        printf("tail [-n NUM] [file]: Display last NUM lines (default 10).\n");
        printf("Usage: tail -n 5 file.txt\n");
    } else if (!strcmp(cmd, "sort")) {
        printf("sort [file]: Sort file lines alphabetically.\n");
        printf("Usage: sort file.txt\n");
    } else if (!strcmp(cmd, "uniq")) {
        printf("uniq [file]: Remove consecutive duplicate lines.\n");
        printf("Usage: uniq file.txt\n");
    } else if (!strcmp(cmd, "whoami")) {
        printf("whoami: Display current user name.\n");
    } else if (!strcmp(cmd, "password")) {
        printf("password: Set or change system password.\n");
        printf("Usage: password\n");
        printf("Note: If password is set, you'll be prompted for old password first.\n");
    } else if (!strcmp(cmd, "ls")) {
        printf("ls [-al] [dir]: List directory contents.\n");
        printf("  -a    Show hidden files (starting with .)\n");
        printf("  -l    Long format with sizes and types\n");
    } else if (!strcmp(cmd, "help")) {
        printf("help [cmd]: Detailed info on commands.\n");
    } else if (!strcmp(cmd, "sysinfo")) {
        printf("sysinfo: Display system information in a colorful, neofetch-like format.\n");
        printf("Shows OS details, memory usage, uptime, and system specs.\n");
    } else if (!strcmp(cmd, "alias")) {
        printf("alias [name[=value]] | [-d name]: Create command shortcuts.\n");
        printf("Usage:\n");
        printf("  alias              # list all aliases\n");
        printf("  alias ll='ls -l'   # create alias\n");
        printf("  alias -d ll        # delete alias\n");
        printf("Examples:\n");
        printf("  alias ll='ls -l'\n");
        printf("  alias la='ls -a'\n");
        printf("  alias ..='cd ..'\n");
    } else if (!strcmp(cmd, "uptime")) {
        printf("uptime: Show how long the system has been running.\n");
        printf("Displays uptime in HH:MM:SS format with days if applicable.\n");
    } else if (!strcmp(cmd, "dmesg")) {
        printf("dmesg: Print kernel boot/debug messages stored in a buffer.\n");
        printf("Shows kernel initialization messages and system events.\n");
    } else if (!strcmp(cmd, "which")) {
        printf("which <command>: Locate a command in the VFS.\n");
        printf("Shows the full path to the specified command if found.\n");
        printf("Usage: which ls, which gcc, etc.\n");
    } else if (!strcmp(cmd, "clear")) {
        printf("clear: Clear the terminal screen.\n");
        printf("Usage: clear\n");
    } else if (!strcmp(cmd, "uname")) {
        printf("uname [flags]: Display system information.\n");
        printf("Flags:\n");
        printf("  -a, --all       Display all information\n");
        printf("  -s, --sysname   Display system name (default)\n");
        printf("  -n, --nodename  Display hostname\n");
        printf("  -r, --release   Display release version\n");
        printf("  -v, --version   Display version info\n");
        printf("  -m, --machine   Display machine type\n");
        printf("Examples:\n");
        printf("  uname            # displays system name\n");
        printf("  uname -a         # displays all info\n");
    } else if (!strcmp(cmd, "exec")) {
        printf("exec <program> [args]: Execute a program directly (replaces shell).\n");
        printf("Usage: exec /path/to/program arg1 arg2\n");
        printf("Note: This will execute the program and return to shell if it exits.\n");
    } else {
        printf("Unknown command.\n");
    }

    return 0;
}