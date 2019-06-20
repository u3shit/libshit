#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/utime.h>

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>

#define ERRNO_MASK 0xff

#define GEN_CONVERT(suf, retval) \
  static int ConvertErrno##suf(int ret)  \
  {                                      \
    if (ret < 0)                         \
    {                                    \
      errno = ret & ERRNO_MASK;          \
      return -1;                         \
    }                                    \
    return retval;                       \
  }
GEN_CONVERT(, ret);
GEN_CONVERT(0, 0);
#undef GEN_CONVERT

// random io functions
ssize_t read(int fd, void* buf, size_t c)
{ return ConvertErrno(sceIoRead(fd, buf, c)); }
ssize_t write(int fd, void* buf, size_t c)
{ return ConvertErrno(sceIoWrite(fd, buf, c)); }

ssize_t pread(int fd, void* buf, size_t count, off_t off)
{ return ConvertErrno(sceIoPread(fd, buf, count, off)); }
ssize_t pwrite(int fd, const void* buf, size_t count, off_t off)
{ return ConvertErrno(sceIoPwrite(fd, buf, count, off)); }

int fseeko(FILE* f, off_t off, int whence)
{
  // SCE_SEEK_* and SEEK_* have the same values
  return ConvertErrno0(sceIoLseek(fileno(f), off, whence));
}
off_t ftello(FILE* f)
{ return ConvertErrno(sceIoLseek(fileno(f), 0, SCE_SEEK_SET)); }

int mkdir(const char* name, mode_t mode)
{ return ConvertErrno0(sceIoMkdir(name, mode)); }

int utime(const char* fname, const struct utimbuf* times)
{
  if (times == 0)
    return ConvertErrno0(utimes(fname, 0));

  struct timeval ts[2];
  ts[0].tv_sec = times->actime;
  ts[0].tv_usec = 0;
  ts[1].tv_sec = times->modtime;
  ts[1].tv_usec = 0;
  return ConvertErrno0(utimes(fname, ts));
}

// time
int nanosleep(const struct timespec* req, struct timespec* rem)
{
  if (rem) memset(rem, 0, sizeof(*rem));
  return ConvertErrno0(
    sceKernelDelayThread(req->tv_sec * 100000 + req->tv_nsec / 1000));
}


// fix binary pthread
int* __errno() { return &errno; }
void* vitasdk_get_tls_data(SceUID thid)
{ return sceKernelGetThreadTLSAddr(thid, 0x361b79e4); }
void* vitasdk_get_pthread_data(SceUID thid)
{ return sceKernelGetThreadTLSAddr(thid, 0x361b79e5); }
int vitasdk_delete_thread_reent(int thid) { return 0; }

int sceKernelLibcGettimeofday(struct timeval* tv, void* tz);
int gettimeofday(struct timeval* tv, void* tz)
{ return ConvertErrno0(sceKernelLibcGettimeofday(tv, tz)); }

// fix binary libsupc++
void* __dso_handle __attribute__((__visibility__("hidden"))) = &__dso_handle;
// http://infocenter.arm.com/help/topic/com.arm.doc.ihi0041e/IHI0041E_cppabi.pdf
int __aeabi_atexit(void*, void (*)(void), void*) __attribute__((noreturn));
int atexit(void (*f)(void)) { return __aeabi_atexit(NULL, f, __dso_handle); }
// __gnu_cxx::__verbose_terminate_handler: override, because binary uses
// reentrant shit
void _ZN9__gnu_cxx27__verbose_terminate_handlerEv(void) { abort(); }


// from libc++
//===--------------------- support/ibm/xlocale.h -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
int vasprintf(char **strp, const char *fmt, va_list ap)
{
  const size_t buff_size = 256;
  size_t str_size;
  if ((*strp = (char *)malloc(buff_size)) == NULL)
  {
    return -1;
  }
  if ((str_size = vsnprintf(*strp, buff_size, fmt,  ap)) >= buff_size)
  {
    if ((*strp = (char *)realloc(*strp, str_size + 1)) == NULL)
    {
      return -1;
    }
    str_size = vsnprintf(*strp, str_size + 1, fmt,  ap);
  }
  return str_size;
}
