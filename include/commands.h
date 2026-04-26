/* after we get a functioning fs and elf this will be depricated */

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
void nano_help();
int cmd_echo(int argc, char** argv);
int cmd_color(int argc, char** argv);
int cmd_memory(int argc, char** argv);
int cmd_free(int argc, char** argv);
int cmd_top(int argc, char** argv);
int cmd_vmstat(int argc, char** argv);
int cmd_snake(int argc, char** argv);
int cmd_vi(int argc, char** argv);
void cmd_clear();
int cmd_pwd(int argc, char** argv);
int cmd_date(int argc, char** argv);
int cmd_rm(int argc, char** argv);
int cmd_cp(int argc, char** argv);
int cmd_mv(int argc, char** argv);
int cmd_shutdown(int argc, char** argv);
int cmd_reboot(int argc, char** argv);
int cmd_calc(int argc, char** argv);
int cmd_man(int argc, char** argv);
int cmd_wc(int argc, char** argv);
int cmd_head(int argc, char** argv);
int cmd_tail(int argc, char** argv);
int cmd_sort(int argc, char** argv);
int cmd_uniq(int argc, char** argv);
int cmd_whoami(int argc, char** argv);
int cmd_password(int argc, char** argv);
int cmd_sysinfo(int argc, char** argv);
int cmd_gcc(int argc, char** argv);
int cmd_alias(int argc, char** argv);
int cmd_uptime(int argc, char** argv);
int cmd_dmesg(int argc, char** argv);
int cmd_which(int argc, char** argv);
int cmd_uname(int argc, char** argv);

void sh(void);

#endif
