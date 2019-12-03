#ifndef GUARD_UNCYNICALLY_NOA_CONVALLASAPONIN_ANTICROSSES_8395
#define GUARD_UNCYNICALLY_NOA_CONVALLASAPONIN_ANTICROSSES_8395
#pragma once

#include "libshit/assert.hpp"

#include <atomic>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace Libshit
{

  class RefCounted
  {
  public:
    RefCounted() = default;
    RefCounted(const RefCounted&) = delete;
    void operator=(const RefCounted&) = delete;
    virtual ~RefCounted() = default;

    virtual void Dispose() noexcept {}

    unsigned use_count() const // emulate boost refcount
    { return strong_count.load(std::memory_order_relaxed); }
    unsigned weak_use_count() const
    { return weak_count.load(std::memory_order_relaxed); }

    void AddRef()
    {
      LIBSHIT_ASSERT(use_count() >= 1);
      strong_count.fetch_add(1, std::memory_order_relaxed);
    }
    void RemoveRef()
    {
      if (strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
      {
        LIBSHIT_ASSERT(weak_use_count() > 0);
        Dispose();
        LIBSHIT_ASSERT(weak_use_count() > 0);
        RemoveWeakRef();
      }
    }

    void AddWeakRef()
    {
      LIBSHIT_ASSERT(weak_use_count() >= 1);
      weak_count.fetch_add(1, std::memory_order_relaxed);
    }
    void RemoveWeakRef()
    {
      if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        delete this;
    }
    bool LockWeak()
    {
      auto count = strong_count.load(std::memory_order_relaxed);
      do
        if (count == 0) return false;
      while (!strong_count.compare_exchange_weak(
               count, count+1,
               std::memory_order_acq_rel, std::memory_order_relaxed));
      return true;
    }

  private:
    template <typename T> friend class RefCountedStackHolder;

    // every object has an implicit weak_count, removed when removing last
    // strong ref
    std::atomic<std::uint_least32_t> weak_count{1}, strong_count{1};
  };

  template <typename T>
  constexpr bool IS_REFCOUNTED = std::is_base_of<RefCounted, T>::value;


  /**
   * Helper class that can be used to place a RefCounted object on the
   * stack/inside a struct/etc. Upon destruction it will assert that there are
   * no shared/weak pointers to it. (In release mode, it will just let the app
   * crash randomly in this case...)
   */
  template <typename T>
  class RefCountedStackHolder
  {
  public:
    static_assert(IS_REFCOUNTED<T>);

    template <typename... Args>
    RefCountedStackHolder(Args&&... args)
      noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
      : t(std::forward<Args>(args)...) {}

    ~RefCountedStackHolder() noexcept
    {
      // decrease strong count even in release mode, so Dispose will see
      // strong_count == 0 (resurrecting is not supported by current RefCounted
      // but whatever)
      auto c = t.strong_count.fetch_sub(1, std::memory_order_acq_rel);
      LIBSHIT_ASSERT_MSG(
        c == 1, "RefCountedStackHolder: strong references remain");
      t.Dispose();

      // we're dead anyway, we don't have to update the weak_count
      LIBSHIT_ASSERT_MSG(
        t.weak_count.load(std::memory_order_acquire) == 1,
        "RefCountedStackHolder: weak references remain");
    }

    T& operator*() noexcept { return t; }
    const T& operator*() const noexcept { return t; }
    T* operator->() noexcept { return &t; }
    const T* operator->() const noexcept { return &t; }

  private:
    T t;
  };
}

#endif
