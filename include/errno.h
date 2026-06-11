/* 
 * MYUNIXLIKEOS errno header
 * Provides POSIX-compliant error code definitions for system calls.
 */
#ifndef _ERRNO_H
#define _ERRNO_H

#include "3rdparty/errno-base.h"

/* Global errno variable - set by syscalls on error */
extern int errno;

/* Convenience macro for setting errno */
#define SET_ERRNO(val) (errno = (val))

/* Get the error message for a given errno value */
const char* strerror(int err);

#endif
