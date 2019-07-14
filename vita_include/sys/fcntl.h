#pragma once

#include_next <sys/fcntl.h>

#undef O_RDONLY
#define O_RDONLY SCE_O_RDONLY
#undef O_WRONLY
#define O_WRONLY SCE_O_WRONLY
#undef O_RDWR
#define O_RDWR SCE_O_RDWR
#undef O_APPEND
#define O_APPEND SCE_O_APPEND
#undef O_CREAT
#define O_CREAT SCE_O_CREAT
#undef O_TRUNC
#define O_TRUNC SCE_O_TRUNC
#undef O_EXCL
#define O_EXCL SCE_O_EXCL
#undef O_SYNC
#undef O_NONBLOCK
#define O_NONBLOCK SCE_O_NBLOCK
