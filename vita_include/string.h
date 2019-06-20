#pragma once

#include_next <string.h>

#include <psp2/kernel/clib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define strnlen sceClibStrnlen

typedef int errno_t;
errno_t strerror_s(char*, size_t, errno_t);

#ifdef __cplusplus
}
#endif
