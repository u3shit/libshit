#ifndef GUARD_UNREVEALINGLY_SLATY_MICROSCHIZONT_AUTOIONIZES_2256
#define GUARD_UNREVEALINGLY_SLATY_MICROSCHIZONT_AUTOIONIZES_2256
#pragma once

#include "libshit/platform.hpp"
#include "libshit/utils.hpp"

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

namespace Libshit
{

  struct LowIo
  {
    using FdType = std::conditional_t<LIBSHIT_OS_IS_WINDOWS, void*, int>;
    static inline const FdType INVALID_FD = reinterpret_cast<FdType>(-1);
    using FilePosition = std::uint64_t;

    LowIo() noexcept = default;
    explicit LowIo(FdType fd) noexcept : fd{fd} {}
    LowIo(FdType fd, bool owning) noexcept : fd{fd}, owning{owning} {}

    enum class Permission { READ_ONLY, WRITE_ONLY, READ_WRITE };
    enum class Mode { OPEN_ONLY, CREATE_ONLY, OPEN_OR_CREATE, TRUNC_OR_CREATE };

#if LIBSHIT_OS_IS_WINDOWS
    LowIo(const wchar_t* fname, Permission perm, Mode mode);
#endif
    LowIo(const char* fname, Permission perm, Mode mode);
    ~LowIo() noexcept;

    static LowIo OpenStdOut();

    LowIo(LowIo&& o) noexcept
      : fd{o.fd},
#if LIBSHIT_OS_IS_WINDOWS
        mmap_fd{o.mmap_fd},
#endif
        owning{o.owning}
    {
      o.fd = INVALID_FD;
#if LIBSHIT_OS_IS_WINDOWS
      o.mmap_fd = INVALID_FD;
#endif
    }
    LowIo& operator=(LowIo&& o) noexcept
    {
      this->~LowIo();
      new (this) LowIo{Move(o)};
      return *this;
    }

    FilePosition GetSize() const;
    void Truncate(FilePosition size) const;
    void PrepareMmap(bool write);
    void* Mmap(FilePosition offs, std::size_t size, bool write) const;
    static void Munmap(void* ptr, std::size_t len);
    void Pread(void* buf, std::size_t len, FilePosition offs) const;
    void Read(void* buf, std::size_t len) const;

    void Pwrite(const void* buf, std::size_t len, FilePosition offs) const;
    void Write(const void* buf, std::size_t len) const;

    FdType fd = INVALID_FD;
#if LIBSHIT_OS_IS_WINDOWS
    FdType mmap_fd = INVALID_FD;
#endif
    bool owning = true;
  };

}
#endif
