#pragma once
/**
 * The sysfunc interface is deprecated. Anything still using these
 * should be migrated to real system calls. The sysfunc interface
 * exists because it was annoying to add new syscall bindings to
 * newlib, but we're not using newlib anymore, so adding new system
 * calls should be easy.
 */

#include <_cheader.h>

/* Privileged */
#define SIRIUS_SYS_FUNC_SYNC          3
#define SIRIUS_SYS_FUNC_LOGHERE       4
#define SIRIUS_SYS_FUNC_KDEBUG        7
#define SIRIUS_SYS_FUNC_INSMOD        8

/* Unpriviliged */
#define SIRIUS_SYS_FUNC_SETHEAP       9
#define SIRIUS_SYS_FUNC_MMAP         10
#define SIRIUS_SYS_FUNC_THREADNAME   11
#define SIRIUS_SYS_FUNC_SETVGACURSOR 13
#define SIRIUS_SYS_FUNC_SETGSBASE    14
#define SIRIUS_SYS_FUNC_NPROC        15

_Begin_C_Header
extern int sysfunc(int command, char ** args);
_End_C_Header
