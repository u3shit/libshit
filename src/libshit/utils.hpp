#ifndef UUID_BCBF4E5C_155A_4984_902F_4A42237608D8
#define UUID_BCBF4E5C_155A_4984_902F_4A42237608D8
#pragma once

#include "except.hpp"

namespace Libshit
{
  template <typename T, typename U>
  T asserted_cast(U* ptr)
  {
    LIBSHIT_ASSERT_MSG(
      dynamic_cast<T>(ptr) == static_cast<T>(ptr), "U is not T");
    return static_cast<T>(ptr);
  }

  template <typename T, typename U>
  T asserted_cast(U& ref)
  {
#ifndef NDEBUG // gcc shut up about unused typedef
    using raw_t = std::remove_reference_t<T>;
    LIBSHIT_ASSERT_MSG(
      dynamic_cast<raw_t*>(&ref) == static_cast<raw_t*>(&ref), "U is not T");
#endif
    return static_cast<T>(ref);
  }

  template <typename T>
  struct AddConst { using Type = const T; };
  template <typename T>
  struct AddConst<T*> { using Type = typename AddConst<T>::Type* const; };

  template <typename T>
  inline T implicit_const_cast(typename AddConst<T>::Type x)
  { return const_cast<T>(x); }
}

#endif
