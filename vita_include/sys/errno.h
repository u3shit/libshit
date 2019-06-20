#pragma once

#define _REENT_ONLY
#include_next <sys/errno.h>
#undef _REENT_ONLY

#ifdef __cplusplus
extern "C"
#endif
int* _sceLibcErrnoLoc();

#undef __errno_r
#define errno (*_sceLibcErrnoLoc())
