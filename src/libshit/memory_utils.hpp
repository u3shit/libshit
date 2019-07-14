#ifndef GUARD_REMEDIALLY_ANTITYPAL_SINGABILITY_NEOLOGISES_6967
#define GUARD_REMEDIALLY_ANTITYPAL_SINGABILITY_NEOLOGISES_6967
#pragma once

#include <cstdlib>
#include <memory> // IWYU pragma: export
#include <type_traits>
#include <utility>

#include "libshit/not_null.hpp" // IWYU pragma: export

namespace Libshit
{

  template <typename T, typename Deleter = std::default_delete<T>>
  using NotNullUniquePtr = NotNull<std::unique_ptr<T, Deleter>>;

  class UninitializedTag {};
  inline const UninitializedTag uninitialized;
  /**
   * Replacement std::make_unique that doesn't value initialize array members.
   * @tparam T make a std::unique_ptr out of this. It should be an array of
   *  unspecified size (e.g. `int[]`).
   * @param n length of the array to make.
   * @return an std::unique_ptr to the allocated array.
   * @throws std::bad_alloc if there's not enough memory.
   * @throws anything that T's constructor throws.
   */
  template <typename T, typename Enable = std::enable_if_t<
                          std::is_array_v<T>>>
  inline NotNullUniquePtr<T> MakeUnique(std::size_t n, UninitializedTag)
  { return NotNullUniquePtr<T>{new typename std::remove_extent<T>::type[n]}; }

  template <typename T, typename... Args, typename Enable = std::enable_if_t<
                                            !std::is_array_v<T>>>
  inline NotNullUniquePtr<T> MakeUnique(Args&&... args)
  { return NotNullUniquePtr<T>{new T(std::forward<Args>(args)...)}; }


  // Deleter that uses free instead of delete. Doesn't call destructors!
  struct FreeDeleter
  {
    void operator()(void* x) { std::free(x); }
  };

}

#endif
