#ifndef GUARD_DREADLESSLY_BALLS_DEEP_DARK_L_FLOORS_0247
#define GUARD_DREADLESSLY_BALLS_DEEP_DARK_L_FLOORS_0247
#pragma once

#include_next <sys/stat.h>
#include <psp2/io/stat.h>

#define lstat stat // no lstat (no symlinks at all?)

#undef _IFMT
#define _IFMT SCE_S_IFMT
#undef _IFDIR
#define _IFDIR SCE_S_IFDIR
#undef _IFCHR
#define _IFCHR 0x8000 // not supported
#undef _IFBLK
#define _IFBLK 0x8000 // not supported
#undef _IFREG
#define _IFREG SCE_S_IFREG
#undef _IFLNK
#define _IFLNK SCE_S_IFLNK
#undef _IFSOCK
#define _IFSOCK 0x8000 // not supported
#undef _IFIFO
#define _IFIFO 0x8000 // not supported

#endif
