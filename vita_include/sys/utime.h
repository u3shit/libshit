#pragma once

#include <sys/time.h>

struct utimbuf
{
  time_t actime;
  time_t modtime;
};

int utime(const char* fname, const struct utimbuf* times)
  __attribute__((nothrow));
