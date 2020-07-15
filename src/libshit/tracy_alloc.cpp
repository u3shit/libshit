#include "libshit/platform.hpp"

#include <Tracy.hpp>

#include <cstdlib>
#include <new>

// unfortunately asan on linux creates link errors if we try to override global
// alloc funcs
#if !LIBSHIT_HAS_ASAN

static constexpr const bool ZONES = false;

void* operator new(std::size_t count)
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    ptr = std::malloc(count);
  }
  if (!ptr) throw std::bad_alloc{};

  TracyAllocS(ptr, count, 5);
  return ptr;
}

void* operator new(std::size_t count, const std::nothrow_t&) noexcept
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    ptr = std::malloc(count);
  }
  TracyAllocS(ptr, count, 5);
  return ptr;
}


void* operator new(std::size_t count, std::align_val_t al)
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    if (posix_memalign(&ptr, static_cast<std::size_t>(al), count))
      throw std::bad_alloc{};
  }
  TracyAllocS(ptr, count, 5);
  return ptr;
}

void* operator new(std::size_t count, std::align_val_t al,
                   const std::nothrow_t&) noexcept
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    if (posix_memalign(&ptr, static_cast<std::size_t>(al), count))
      return nullptr;
  }
  TracyAllocS(ptr, count, 5);
  return ptr;
}


void operator delete(void* ptr) noexcept
{
  TracyFreeS(ptr, 5);
  ZoneNamedNC(x, "delete", 0x003300, ZONES);
  std::free(ptr);
}

void operator delete(void* ptr, std::align_val_t al) noexcept
{
  TracyFreeS(ptr, 5);
  ZoneNamedNC(x, "delete", 0x003300, ZONES);
  std::free(ptr);
}
#endif
