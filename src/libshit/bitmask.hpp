#ifndef GUARD_PRECOLONIALLY_SMOOCHY_RECOUNTMENT_PROTESTS_TOO_MUCH_8222
#define GUARD_PRECOLONIALLY_SMOOCHY_RECOUNTMENT_PROTESTS_TOO_MUCH_8222
#pragma once

#include <type_traits> // IWYU pragma: keep

namespace Libshit
{

#define LIBSHIT_BITMASK_GEN0(opf, ops, T, Res, ass)                         \
  constexpr inline Res operator opf(Res a, T b) noexcept                    \
  {                                                                         \
    return ass static_cast<T>(static_cast<std::underlying_type_t<T>>(a) ops \
                              static_cast<std::underlying_type_t<T>>(b));   \
  }
#define LIBSHIT_BITMASK_GEN1(op, T) \
  LIBSHIT_BITMASK_GEN0(op, op, T, T, ) LIBSHIT_BITMASK_GEN0(op##=, op, T, T&, a=)


  /// Add bitwise operators for an enum class.
  // Not the usual template + enable_if trick, because that doesn't work accross
  // namespaces...
#define LIBSHIT_BITMASK(T)                                               \
  LIBSHIT_BITMASK_GEN1(|, T) LIBSHIT_BITMASK_GEN1(&, T)                  \
  LIBSHIT_BITMASK_GEN1(^, T)                                             \
                                                                         \
  constexpr inline T operator~(T t) noexcept                             \
  { return static_cast<T>(~static_cast<std::underlying_type_t<T>>(t)); } \
  constexpr inline bool Is(T t) noexcept { return t != static_cast<T>(0); }

}

#endif
