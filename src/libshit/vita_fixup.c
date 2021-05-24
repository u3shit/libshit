#include <dirent.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utime.h>

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>

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

char* getenv(const char* name) { return NULL; }

// random io functions
// fcntl.h patched so O_* equals to SCE_O_* defines
int open(const char* fname, int flags, mode_t mode)
{ return ConvertErrno(sceIoOpen(fname, flags, mode)); }
int close(int fd) { return ConvertErrno0(sceIoClose(fd)); }

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

int ftruncate(int fd, off_t len)
{
  SceIoStat buf;
  buf.st_size = len;
  return ConvertErrno0(sceIoChstatByFd(fd, &buf, SCE_CST_SIZE));
}

static int ConvertStat(int ret, struct stat* buf, SceIoStat* sce_buf)
{
  if (ret < 0)
  {
    errno = ret & ERRNO_MASK;
    return -1;
  }

  memset(buf, 0, sizeof(*buf));
  buf->st_mode = sce_buf->st_mode;
  buf->st_size = sce_buf->st_size;
  sceRtcGetTime_t(&sce_buf->st_atime, &buf->st_atime);
  sceRtcGetTime_t(&sce_buf->st_ctime, &buf->st_ctime);
  sceRtcGetTime_t(&sce_buf->st_mtime, &buf->st_mtime);
  return 0;
}

int stat(const char* fname, struct stat* buf)
{
  SceIoStat sce_buf;
  int ret = sceIoGetstat(fname, &sce_buf);
  return ConvertStat(ret, buf, &sce_buf);
}

int fstat(int fd, struct stat* buf)
{
  SceIoStat sce_buf;
  int ret = sceIoGetstatByFd(fd, &sce_buf);
  return ConvertStat(ret, buf, &sce_buf);
}

int unlink(const char* fname)
{ return ConvertErrno0(sceIoRemove(fname)); }

int mkdir(const char* name, mode_t mode)
{ return ConvertErrno0(sceIoMkdir(name, mode)); }

int rmdir(const char* name)
{ return ConvertErrno0(sceIoRmdir(name)); }

// dir funs
struct DIR_
{
  SceUID uid;
  dirent de;
};

DIR* opendir(const char* name)
{
  DIR* dir = malloc(sizeof(DIR));
  if (!dir)
  {
    errno = ENOMEM;
    return NULL;
  }

  int id = sceIoDopen(name);
  if (id < 0)
  {
    free(dir);
    errno = id & ERRNO_MASK;
    return NULL;
  }

  dir->uid = id;
  return dir;
}

int closedir(DIR* dir)
{
  int ret = ConvertErrno0(sceIoDclose(dir->uid));
  free(dir);
  return ret;
}

struct dirent* readdir(DIR* dir)
{
  int res = sceIoDread(dir->uid, &dir->de);
  if (res < 0) errno = res & ERRNO_MASK;
  return res > 0 ? &dir->de : NULL;
}

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

// override abort,_exit: the sceLibc versions locks up the whole console,
// forcing you to reboot it
// todo: what about exit?
void _exit(int status)
{
  fflush(stdout);
  fflush(stderr);
  sceKernelExitProcess(status);
  __builtin_unreachable();
}
void abort() { _exit(1); }


// fix binary pthread
int* __errno() { return &errno; }

#define MAGIC 0x36 // doesn't work if it's too big
void* vitasdk_get_tls_data(SceUID thid)
{
  return thid ? sceKernelGetThreadTLSAddr(thid, MAGIC) :
    sceKernelGetTLSAddr(MAGIC);
}

void* vitasdk_get_pthread_data(SceUID thid)
{
  return thid ? sceKernelGetThreadTLSAddr(thid, MAGIC + 1) :
    sceKernelGetTLSAddr(MAGIC + 1);
}

int vitasdk_delete_thread_reent(int thid) { return 0; }

int sceKernelLibcGettimeofday(struct timeval* tv, void* tz);
int gettimeofday(struct timeval* tv, void* tz)
{ return ConvertErrno0(sceKernelLibcGettimeofday(tv, tz)); }

// fix binary libsupc++
void* __dso_handle __attribute__((__visibility__("hidden"))) = &__dso_handle;
// http://infocenter.arm.com/help/topic/com.arm.doc.ihi0041e/IHI0041E_cppabi.pdf
int __aeabi_atexit(void*, void (*)(void), void*);
int atexit(void (*f)(void)) { return __aeabi_atexit(NULL, f, __dso_handle); }
// __gnu_cxx::__verbose_terminate_handler: override, because binary uses
// reentrant shit
void _ZN9__gnu_cxx27__verbose_terminate_handlerEv(void) { printf("verbtermh\n"); abort(); }


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

// init/deinit c++ stuff
typedef void (**InitPtr)(void);
extern void (*__preinit_array_start []) (void) __attribute__((weak));
extern void (*__preinit_array_end []) (void) __attribute__((weak));
extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));
extern void (*__fini_array_start [])(void) __attribute__((weak));
extern void (*__fini_array_end [])(void) __attribute__((weak));

static void cleanup(void)
{
  for (InitPtr ptr = __fini_array_end; ptr > __fini_array_start; )
    (*--ptr)();
  pthread_terminate();

  fflush(stdout);
  fflush(stderr);
}

void LibshitInitVita(void)
{
  atexit(cleanup);

  pthread_init();
  for (InitPtr ptr = __preinit_array_start; ptr != __preinit_array_end; ++ptr)
    (*ptr)();
  for (InitPtr ptr = __init_array_start; ptr != __init_array_end; ++ptr)
    (*ptr)();
}
