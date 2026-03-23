#include "commands.h"
#include "vfs.h"
#include "string.h"

extern void terminal_write(const char*);
extern char fs_type_name[16];

int cmd_fs(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("fs: select NTFS FAT32 exFAT EXT4 APFS\n");
        return -1;
    }

    const char* name = argv[1];
    if (!(strcmp(name, "NTFS") == 0 || strcmp(name, "FAT32") == 0 ||
          strcmp(name, "exFAT") == 0 || strcmp(name, "EXT4") == 0 ||
          strcmp(name, "APFS") == 0)) {
        terminal_write("fs: unknown type\n");
        return -1;
    }

    for (int i = 0; i < 15 && name[i]; i++) {
        fs_type_name[i] = name[i];
    }
    fs_type_name[15] = '\0';

    terminal_write("File system set to: ");
    terminal_write(fs_type_name);
    terminal_write("\n");
    return 0;
}
