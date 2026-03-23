#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include "vfs.h"

typedef int (*command_fn_t)(int argc, char** argv);

int compare_string(const char* a, const char* b);
void copy_string(char* dest, const char* src);

int cmd_ls(int argc, char** argv);
int cmd_mkdir(int argc, char** argv);
int cmd_touch(int argc, char** argv);
int cmd_cd(int argc, char** argv);
int cmd_cat(int argc, char** argv);
int cmd_grep(int argc, char** argv);
int cmd_fs(int argc, char** argv);
int cmd_help(int argc, char** argv);
int cmd_nano(int argc, char** argv);
int cmd_echo(int argc, char** argv);
int cmd_vi(int argc, char** argv);

#endif
