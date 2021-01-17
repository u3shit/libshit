#ifndef GUARD_NONCONSTAT_SELF_JUSTIFICATORY_LASIK_PRECONCEIVES_0053
#define GUARD_NONCONSTAT_SELF_JUSTIFICATORY_LASIK_PRECONCEIVES_0053
#pragma once

#include "libshit/utils.hpp"

#include <type_traits> // IWYU pragma: keep
#include <utility>

namespace Libshit
{

  /**
   * Like std::unique_ptr, but works with non-pointers too.
   * @see ManagedObject for a simplified interface.
   */
  template <typename T, typename Traits>
  class BasicManagedObject
  {
  public:
    static_assert(std::is_nothrow_copy_constructible_v<T>);
    static_assert(std::is_nothrow_swappable_v<T>);

    constexpr BasicManagedObject(T t) noexcept : t{Libshit::Move(t)} {}
    ~BasicManagedObject() noexcept { Reset(); }

    constexpr BasicManagedObject(BasicManagedObject&& o) noexcept : t{o.t}
    { o.t = Traits::GetNull(); }
    BasicManagedObject(const BasicManagedObject&) = delete;

    constexpr BasicManagedObject& operator=(BasicManagedObject o) noexcept
    {
      using std::swap;
      swap(t, o.t);
      return *this;
    }

    constexpr const T& Get() const noexcept { return t; }
    constexpr T& Get() noexcept { return t; }

    constexpr explicit operator bool() const noexcept
    { return t != Traits::GetNull(); }

    T Release() noexcept { return std::exchange(t, Traits::GetNull()); }
    void Reset() noexcept
    {
      if (t != Traits::GetNull()) Traits::Delete(t);
      t = Traits::GetNull();
    }

  private:
    T t;
  };

  template <typename T, auto Func, T Null = 0>
  struct ManagedObjectTraits
  {
    constexpr static void Delete(T& t) noexcept { Func(t); }
    constexpr static T GetNull() noexcept { return Null; }
  };

  template <typename T, auto Func, T Null = 0>
  using ManagedObject = BasicManagedObject<T, ManagedObjectTraits<T, Func, Null>>;

}

#endif
