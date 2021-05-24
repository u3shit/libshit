#include "libshit/random.hpp"

#include "libshit/except.hpp"
#include "libshit/platform.hpp"

#if LIBSHIT_OS_IS_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
extern "C" BOOLEAN NTAPI SystemFunction036(PVOID, ULONG);
#elif LIBSHIT_OS_IS_VITA
#  include <psp2/kernel/rng.h>
#else
#  include <unistd.h>
#endif

namespace Libshit
{
  void FillRandom(Span<std::byte> buf)
  {
#if LIBSHIT_OS_IS_WINDOWS
    if (!SystemFunction036(buf.data(), buf.size()))
      LIBSHIT_THROW_WINERROR("SystemFunction036");
#elif LIBSHIT_OS_IS_VITA
    if (auto ret = sceKernelGetRandomNumber(buf.data(), buf.size()); ret < 0)
      LIBSHIT_THROW(Libshit::ErrnoError, ret & 0xff,
                    "API function", "sceKernelGetRandomNumber",
                    "Vita error code", ret);
#else
    if (getentropy(buf.data(), buf.size()) < 0)
      LIBSHIT_THROW_ERRNO("getentropy");
#endif
  }
}
