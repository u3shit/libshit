#define _FILE_OFFSET_BITS 64

#include "libshit/low_io.hpp"

#include "libshit/assert.hpp"
#include "libshit/except.hpp"

#include <Tracy.hpp>

#include <cstdlib>

#if LIBSHIT_OS_IS_WINDOWS
#  include "libshit/wtf8.hpp"

#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <cstdio>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  if !LIBSHIT_OS_IS_VITA
#    include <sys/mman.h>
#  endif
#endif

#define LIBSHIT_LOG_NAME "low_io"
#include "libshit/logger_helper.hpp"

namespace Libshit
{

#if LIBSHIT_OS_IS_WINDOWS

  static int Perm2Access(LowIo::Permission perm)
  {
    switch (perm)
    {
    case LowIo::Permission::READ_ONLY:  return GENERIC_READ; break;
    case LowIo::Permission::WRITE_ONLY: return GENERIC_WRITE; break;
    case LowIo::Permission::READ_WRITE:
      return GENERIC_READ | GENERIC_WRITE; break;
    }
    LIBSHIT_UNREACHABLE("Invalid permission");
  }

  static int Mode2Disposition(LowIo::Mode mode)
  {
    switch (mode)
    {
    case LowIo::Mode::OPEN_ONLY:       return OPEN_EXISTING;
    case LowIo::Mode::CREATE_ONLY:     return CREATE_NEW;
    case LowIo::Mode::OPEN_OR_CREATE:  return OPEN_ALWAYS;
    case LowIo::Mode::TRUNC_OR_CREATE: return CREATE_ALWAYS;
    }
    LIBSHIT_UNREACHABLE("Invalid mode");
  }

  LowIo::LowIo(const wchar_t* fname, Permission perm, Mode mode)
  {
    if (auto [err, werr] = TryOpen(fname, perm, mode); err != OpenError::OK)
      LIBSHIT_LOWIO_RETHROW_OPEN_ERROR(werr);
  }

  std::pair<LowIo::OpenError, LowIo::ErrorCode> LowIo::TryOpen(
    const wchar_t* fname, Permission perm, Mode mode)
  {
    fd = CreateFileW(
        fname, Perm2Access(perm), FILE_SHARE_DELETE | FILE_SHARE_READ, nullptr,
        Mode2Disposition(mode), 0, nullptr);
    if (fd != INVALID_HANDLE_VALUE) return { OpenError::OK, 0 };
    switch (auto err = GetLastError())
    {
    case ERROR_ACCESS_DENIED:  return { OpenError::ACCESS, err };
    case ERROR_FILE_EXISTS:    return { OpenError::EXISTS, err };
    case ERROR_FILE_NOT_FOUND: return { OpenError::NOT_EXISTS, err };
    default: return { OpenError::UNKNOWN, err };
    }
  }

  LowIo::LowIo(const char* fname, Permission perm, Mode mode)
    : LowIo{Wtf8ToWtf16Wstr(fname).c_str(), perm, mode} {}

  std::pair<LowIo::OpenError, LowIo::ErrorCode> LowIo::TryOpen(
    const char* fname, Permission perm, Mode mode)
  { return TryOpen(Wtf8ToWtf16Wstr(fname).c_str(), perm, mode); }

  LowIo LowIo::OpenStdOut()
  {
    auto h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || h == nullptr)
      LIBSHIT_THROW_WINERROR("GetStdHandle");
    HANDLE ret;
    if (DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &ret, 0,
                        FALSE, DUPLICATE_SAME_ACCESS) == 0)
      LIBSHIT_THROW_WINERROR("DuplicateHandle");
    return LowIo{ret};
  }

  void LowIo::Reset() noexcept
  {
    if (mmap_fd != INVALID_FD)
    {
      CloseHandle(mmap_fd);
      mmap_fd = INVALID_FD;
    }

    if (owning && fd != INVALID_FD)
    {
      CloseHandle(fd);
      fd = INVALID_FD; owning = false;
    }
  }

  LowIo::FilePosition LowIo::GetSize() const
  {
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fd, &size)) LIBSHIT_THROW_WINERROR("GetFileSizeEx");
    return size.QuadPart;
  }

  void LowIo::Truncate(FilePosition size) const
  {
    LARGE_INTEGER zero{};
    LARGE_INTEGER old_pos;
    if (!SetFilePointerEx(fd, zero, &old_pos, FILE_CURRENT))
      LIBSHIT_THROW_WINERROR("SetFilePointerEx");
    LARGE_INTEGER win_size;
    win_size.QuadPart = size;
    if (!SetFilePointerEx(fd, win_size, nullptr, FILE_BEGIN))
      LIBSHIT_THROW_WINERROR("SetFilePointerEx");
    if (!SetEndOfFile(fd)) LIBSHIT_THROW_WINERROR("SetEndOfFile");
    if (!SetFilePointerEx(fd, old_pos, nullptr, FILE_BEGIN))
      LIBSHIT_THROW_WINERROR("SetFilePointerEx");
  }

  void LowIo::PrepareMmap(bool write)
  {
    mmap_fd = CreateFileMapping(
      fd, NULL, write ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
    if (mmap_fd == nullptr) LIBSHIT_THROW_WINERROR("CreateFileMapping");
  }

  LowIo::MmapPtr LowIo::Mmap(
    FilePosition offs, std::size_t size, bool write) const
  {
    auto ret = MapViewOfFile(mmap_fd, write ? FILE_MAP_WRITE : FILE_MAP_READ,
                             offs >> 16 >> 16, offs, size);
    if (ret == nullptr)
      LIBSHIT_THROW_WINERROR(
        "MapViewOfFile", "Mmap offset", offs, "Mmap size", size);
    TracyAllocNS(ret, size, 5, "mmap");
    return {ret};
  }

  void LowIo::Munmap(void* ptr, std::size_t size)
  {
    if (!UnmapViewOfFile(ptr))
    {
      auto err = GetLastError();
      ERR << "UnmapViewOfFile failed, aborting: "
          << Libshit::GetWindowsError(err) << std::endl;
      std::abort();
    }
    TracyFreeNS(ptr, 5, "mmap");
  }

  void LowIo::MmapPtr::Reset() noexcept
  {
    if (!ptr) return;
    if (!UnmapViewOfFile(ptr))
    {
      auto err = GetLastError();
      ERR << "UnmapViewOfFile failed, aborting: "
          << Libshit::GetWindowsError(err) << std::endl;
      std::abort();
    }
    TracyFreeNS(ptr, 5, "mmap");
    ptr = nullptr;
  }

  void LowIo::Pread(void* buf, std::size_t len, FilePosition offs) const
  {
    DWORD size;
    OVERLAPPED o{};
    o.Offset = offs;
    o.OffsetHigh = offs >> 16 >> 16;

    if (!ReadFile(fd, buf, len, &size, &o) || size != len)
      LIBSHIT_THROW_WINERROR("ReadFile");
  }

  void LowIo::Read(void* buf, std::size_t len) const
  {
    DWORD size;
    if (!ReadFile(fd, buf, len, &size, nullptr) || size != len)
      LIBSHIT_THROW_WINERROR("ReadFile");
  }

  void LowIo::Pwrite(const void* buf, std::size_t len, FilePosition offs) const
  {
    DWORD size;
    OVERLAPPED o{};
    o.Offset = offs;
    o.OffsetHigh = offs >> 16 >> 16;

    if (!WriteFile(fd, buf, len, &size, &o) || size != len)
      LIBSHIT_THROW_WINERROR("WriteFile");
  }

  void LowIo::Write(const void* buf, std::size_t len) const
  {
    DWORD size;
    if (!WriteFile(fd, buf, len, &size, nullptr) || size != len)
      LIBSHIT_THROW_WINERROR("WriteFile");
  }

#else // linux/unix

  static int Perm2Flags(LowIo::Permission perm)
  {
    switch (perm)
    {
    case LowIo::Permission::READ_ONLY:  return O_RDONLY; break;
    case LowIo::Permission::WRITE_ONLY: return O_WRONLY; break;
    case LowIo::Permission::READ_WRITE: return O_RDWR;   break;
    }
    LIBSHIT_UNREACHABLE("Invalid permission");
  }

  static int Mode2Flags(LowIo::Mode mode)
  {
    switch (mode)
    {
    case LowIo::Mode::OPEN_ONLY:       return 0;
    case LowIo::Mode::CREATE_ONLY:     return O_CREAT | O_EXCL;
    case LowIo::Mode::OPEN_OR_CREATE:  return O_CREAT;
    case LowIo::Mode::TRUNC_OR_CREATE: return O_CREAT | O_TRUNC;
    }
    LIBSHIT_UNREACHABLE("Invalid mode");
  }

  LowIo::LowIo(const char* fname, Permission perm, Mode mode)
  {
    if (auto [error, errno_sav] = TryOpen(fname, perm, mode);
        error != OpenError::OK)
      LIBSHIT_LOWIO_RETHROW_OPEN_ERROR(errno_sav);
  }

  std::pair<LowIo::OpenError, LowIo::ErrorCode> LowIo::TryOpen(
    const char* fname, Permission perm, Mode mode)
  {
    fd = open(
      fname, Perm2Flags(perm) | Mode2Flags(mode)
      LIBSHIT_OS_NOT_VITA(| O_CLOEXEC | O_NOCTTY), 0666);
    if (fd != -1) return { OpenError::OK, 0 };

    switch (errno)
    {
    case EACCES: return { OpenError::ACCESS, EACCES };
    case EEXIST: return { OpenError::EXISTS, EEXIST };
    case ENOENT: return { OpenError::NOT_EXISTS, ENOENT };
    default: return { OpenError::UNKNOWN, errno };
    }
  }

  LowIo LowIo::OpenStdOut()
  {
#if LIBSHIT_OS_IS_VITA
    // TODO: does this even work?!
    int fd = dup(STDOUT_FILENO);
#else
    int fd = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
#endif
    if (fd == -1) LIBSHIT_THROW_ERRNO("dup");
    return LowIo{fd};
  }

  void LowIo::Reset() noexcept
  {
    if (owning && fd != INVALID_FD)
    {
      if (close(fd)) perror("close");
      fd = INVALID_FD; owning = false;
    }
  }

  LowIo::FilePosition LowIo::GetSize() const
  {
    struct stat buf;
    if (fstat(fd, &buf) < 0) LIBSHIT_THROW_ERRNO("fstat");
    return buf.st_size;
  }

  void LowIo::Truncate(FilePosition size) const
  {
    if (ftruncate(fd, size) < 0) LIBSHIT_THROW_ERRNO("ftruncate");
  }

  void LowIo::PrepareMmap(bool) {}

  LowIo::MmapPtr LowIo::Mmap(
    FilePosition offs, std::size_t size, bool write) const
  {
#if LIBSHIT_OS_IS_VITA
    errno = ENOSYS;
    LIBSHIT_THROW_ERRNO("mmap");
#else
    auto ptr = mmap(
      nullptr, size, write ? PROT_WRITE : PROT_READ,
      write ? MAP_SHARED : MAP_PRIVATE, fd, offs);
    if (ptr == MAP_FAILED) LIBSHIT_THROW_ERRNO("mmap");
    TracyAllocNS(ptr, size, 5, "mmap");
    return {ptr, size};
#endif
  }

  void LowIo::Munmap(void* ptr, std::size_t size)
  {
#if !LIBSHIT_OS_IS_VITA
    TracyFreeNS(ptr, 5, "mmap");
    if (munmap(ptr, size))
    {
      auto err = errno;
      ERR << "munmap failed, aborting: " << Libshit::GetErrnoError(err)
          << std::endl;
      std::abort();
    }
#endif
  }

  void LowIo::MmapPtr::Reset() noexcept
  {
#if !LIBSHIT_OS_IS_VITA
    if (!ptr) return;
    TracyFreeNS(ptr, 5, "mmap");
    if (munmap(ptr, size))
    {
      auto err = errno;
      ERR << "munmap failed, aborting: " << Libshit::GetErrnoError(err)
          << std::endl;
      std::abort();
    }
    ptr = nullptr;
    size = 0;
#endif
  }

  void LowIo::Pread(void* buf, std::size_t len, FilePosition offs) const
  {
    if (pread(fd, buf, len, offs) != len) LIBSHIT_THROW_ERRNO("pread");
  }

  void LowIo::Read(void* buf, std::size_t len) const
  {
    if (read(fd, buf, len) != len) LIBSHIT_THROW_ERRNO("read");
  }

  void LowIo::Pwrite(const void* buf, std::size_t len, FilePosition offs) const
  {
    if (pwrite(fd, buf, len, offs) != len) LIBSHIT_THROW_ERRNO("pwrite");
  }

  void LowIo::Write(const void* buf, std::size_t len) const
  {
    if (write(fd, buf, len) != len) LIBSHIT_THROW_ERRNO("write");
  }

#endif
}
