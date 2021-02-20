#ifndef GUARD_UNREVEALINGLY_SLATY_MICROSCHIZONT_AUTOIONIZES_2256
#define GUARD_UNREVEALINGLY_SLATY_MICROSCHIZONT_AUTOIONIZES_2256
#pragma once

#include "libshit/platform.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace Libshit
{

  class LowIo
  {
  public:
    using FdType = std::conditional_t<LIBSHIT_OS_IS_WINDOWS, void*, int>;
    static inline const FdType INVALID_FD = reinterpret_cast<FdType>(-1);
    using FilePosition = std::uint64_t;

    LowIo() noexcept {}; // can't be default due to a clang8 bug
    explicit LowIo(FdType fd) noexcept : fd{fd} {}
    LowIo(FdType fd, bool owning) noexcept : fd{fd}, owning{owning} {}

    enum class Permission { READ_ONLY, WRITE_ONLY, READ_WRITE };
    enum class Mode { OPEN_ONLY, CREATE_ONLY, OPEN_OR_CREATE, TRUNC_OR_CREATE };

    LowIo(const char* fname, Permission perm, Mode mode);
    ~LowIo() noexcept { Reset(); }
    void Reset() noexcept;

    static LowIo OpenStdOut();

    enum class OpenError
    {
      OK,
      ACCESS, // (maybe) perm related errors
      EXISTS, NOT_EXISTS, // (maybe) mode related errors
      UNKNOWN // not mapped error
    };
    using ErrorCode = std::conditional_t<
      LIBSHIT_OS_IS_WINDOWS, unsigned int, int>;
    std::pair<OpenError, ErrorCode> TryOpen(
      const char* fname, Permission perm, Mode mode);

#if LIBSHIT_OS_IS_WINDOWS
    LowIo(const wchar_t* fname, Permission perm, Mode mode);
    std::pair<OpenError, ErrorCode> TryOpen(
      const wchar_t* fname, Permission perm, Mode mode);

#  define LIBSHIT_LOWIO_RETHROW_OPEN_ERROR(code) \
    LIBSHIT_THROW(::Libshit::WindowsError, code, "API function", "CreateFile")
#else
#  define LIBSHIT_LOWIO_RETHROW_OPEN_ERROR(code) \
    LIBSHIT_THROW(::Libshit::ErrnoError, code, "API function", "open")
#endif

    LowIo(LowIo&& o) noexcept
      : fd{o.fd}, LIBSHIT_OS_WINDOWS(mmap_fd{o.mmap_fd},) owning{o.owning}
    {
      o.fd = INVALID_FD;
      LIBSHIT_OS_WINDOWS(o.mmap_fd = INVALID_FD);
    }
    LowIo& operator=(LowIo o) noexcept
    {
      swap(*this, o);
      return *this;
    }

    friend void swap(LowIo& a, LowIo& b) noexcept
    {
      std::swap(a.fd, b.fd);
      LIBSHIT_OS_WINDOWS(std::swap(a.mmap_fd, b.mmap_fd));
      std::swap(a.owning, b.owning);
    }

    class MmapPtr
    {
    public:
      MmapPtr() noexcept = default;
      MmapPtr(MmapPtr&& o) noexcept
        : ptr{o.ptr} LIBSHIT_OS_NOT_WINDOWS(, size{o.size})
      {
        o.ptr = nullptr;
        LIBSHIT_OS_NOT_WINDOWS(o.size = 0);
      }
      MmapPtr& operator=(MmapPtr o) noexcept
      {
        swap(*this, o);
        return *this;
      }
      ~MmapPtr() noexcept { Reset(); }

      friend void swap(MmapPtr& a, MmapPtr& b) noexcept
      {
        std::swap(a.ptr, b.ptr);
        LIBSHIT_OS_NOT_WINDOWS(std::swap(a.size, b.size));
      }

      explicit operator bool() const noexcept { return ptr; }
      void* Get() const noexcept { return ptr; }

      void Reset() noexcept;
      void* Release()
      {
        auto res = ptr;
        ptr = nullptr;
        return res;
      }

    private:
#if LIBSHIT_OS_IS_WINDOWS
      MmapPtr(void* ptr) : ptr{ptr} {}
#else
      MmapPtr(void* ptr, std::size_t size) : ptr{ptr}, size{size} {}
#endif
      friend class LowIo;

      void* ptr = nullptr;
      LIBSHIT_OS_NOT_WINDOWS(std::size_t size = 0);
    };

    static constexpr const bool MMAP_SUPPORTED = !LIBSHIT_OS_IS_VITA;

    FilePosition GetSize() const;
    void Truncate(FilePosition size) const;
    void PrepareMmap(bool write);
    MmapPtr Mmap(FilePosition offs, std::size_t size, bool write) const;
    static void Munmap(void* ptr, std::size_t size);

    void Pread(void* buf, std::size_t len, FilePosition offs) const;
    void Read(void* buf, std::size_t len) const;

    void Pwrite(const void* buf, std::size_t len, FilePosition offs) const;
    void Write(const void* buf, std::size_t len) const;

  private:
    FdType fd = INVALID_FD;
    LIBSHIT_OS_WINDOWS(FdType mmap_fd = INVALID_FD);
    bool owning = true;
  };

}
#endif
