#ifndef GUARD_SUGGESTINGLY_TRUMPIST_BURAO_INWRITES_0798
#define GUARD_SUGGESTINGLY_TRUMPIST_BURAO_INWRITES_0798
#pragma once

// dummy impl

#include <errno.h>

struct statvfs
{
  unsigned long f_bsize, f_frsize, f_blocks, f_bfree, f_bavail;
};
static inline int __attribute__((nothrow))
statvfs(const char* path, struct statvfs* buf)
{ errno = ENOSYS; return -1; }

#endif
