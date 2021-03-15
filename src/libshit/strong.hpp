#ifndef GUARD_SOFTHEADEDLY_OVERDIAGNOSED_HYTHERGRAPH_OUTSURVIVES_8503
#define GUARD_SOFTHEADEDLY_OVERDIAGNOSED_HYTHERGRAPH_OUTSURVIVES_8503
#pragma once

#include "libshit/assert.hpp"
#include "libshit/span.hpp"
#include "libshit/utils.hpp"

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace Libshit
{

#define LIBSHIT_OP_CMP_GEN(x) x(==) x(!=) x(<) x(<=) x(>) x(>=)

  template <typename T, typename Tag>
  class StrongTypedef
  {
    using Self = StrongTypedef<T, Tag>;
  public:
    using Type = T;

    constexpr StrongTypedef() = default;
    constexpr explicit StrongTypedef(const T& t)
      noexcept(std::is_nothrow_copy_constructible_v<T>)
      : t(t) {}
    constexpr explicit StrongTypedef(T&& t)
      noexcept(std::is_nothrow_move_constructible_v<T>)
      : t(Libshit::Move(t)) {}

    template <typename U, typename =
              std::enable_if_t<std::is_convertible_v<T, U>>>
    constexpr StrongTypedef(StrongTypedef<U, Tag> o)
      : t(Libshit::Move(static_cast<U>(o))) {}

    constexpr explicit operator T&() noexcept { return t; }
    constexpr explicit operator const T&() const noexcept { return t; }

    template <typename U> constexpr T& Get() noexcept
    { static_assert(std::is_same_v<Self, U>); return t; }
    template <typename U> constexpr const T& Get() const noexcept
    { static_assert(std::is_same_v<Self, U>); return t; }

    constexpr StrongTypedef& operator++() { ++t; return *this; }
    constexpr StrongTypedef operator++(int) { return StrongTypedef{t++}; }
    constexpr StrongTypedef& operator--() { --t; return *this; }
    constexpr StrongTypedef operator--(int) { return StrongTypedef{t--}; }

    constexpr explicit operator bool() const noexcept
    { return static_cast<bool>(t); }

#define LIBSHIT_OP(op)                                             \
    constexpr bool operator op(const StrongTypedef& o) const noexcept \
    { return t op o.t; }
    LIBSHIT_OP_CMP_GEN(LIBSHIT_OP)
#undef LIBSHIT_OP

#define LIBSHIT_OP(op)\
    constexpr StrongTypedef operator op(const StrongTypedef& o) const noexcept \
    { return StrongTypedef{t op o.t}; }                                        \
    constexpr StrongTypedef operator op(StrongTypedef&& o) const noexcept      \
    { return StrongTypedef{t op Libshit::Move(o.t)}; }                         \
    constexpr StrongTypedef operator op(const T& o) const noexcept             \
    { return StrongTypedef{t op o}; }                                          \
    constexpr StrongTypedef operator op(T&& o) const noexcept                  \
    { return StrongTypedef{t op Libshit::Move(o)}; }                           \
    constexpr StrongTypedef& operator op##=(const StrongTypedef& o) noexcept   \
    { t op##= o.t; return *this; }                                             \
    constexpr StrongTypedef& operator op##=(StrongTypedef&& o) noexcept        \
    { t op##= Libshit::Move(o.t); return *this; }                              \
    constexpr friend StrongTypedef operator op(                                \
      const T& t, const StrongTypedef& o) noexcept                             \
    { return StrongTypedef{t op o.t}; }                                        \
    constexpr friend StrongTypedef operator op(                                \
      T&& t, StrongTypedef&& o) noexcept                                       \
    { return StrongTypedef{Libshit::Move(t) op std::move(o.t)}; }
    LIBSHIT_OP(+) LIBSHIT_OP(-) LIBSHIT_OP(*) LIBSHIT_OP(/)
#undef LIBSHIT_OP

    // so this shit can be used with contiguous allocators
    template <typename U>
    constexpr friend U* operator+(U* ptr, const StrongTypedef& s) noexcept
    { return ptr + s.t; }

  private:
    T t;
  };

  template <typename T, typename Tag>
  std::ostream& operator<<(std::ostream& os, const StrongTypedef<T, Tag>& o)
  {
    return os << static_cast<const T&>(o);
  }

  template <typename T, typename Index> class StrongPointer;

  template <typename T, typename IndexT, typename Tag>
  class StrongPointer<T, StrongTypedef<IndexT, Tag>>
  {
    using Index = StrongTypedef<IndexT, Tag>;
    using DiffT = std::make_signed_t<IndexT>;
    using Diff = StrongTypedef<DiffT, Tag>;
  public:
    using element_type = T; // for pointer_traits
    using size_type = Index;
    using difference_type = Diff;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::random_access_iterator_tag;

    constexpr StrongPointer() noexcept = default;
    constexpr StrongPointer(std::nullptr_t) noexcept : t{nullptr} {}
    constexpr StrongPointer(T* t) noexcept : t{t} {}
    template <typename U,
              typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    constexpr StrongPointer(StrongPointer<U, Index> ptr) noexcept
      : t{ptr.Get()} {}

    constexpr T* Get() noexcept { return t; }
    constexpr const T* Get() const noexcept { return t; }

    constexpr T& operator*() noexcept { LIBSHIT_ASSERT(t); return *t; }
    constexpr const T& operator*() const noexcept { LIBSHIT_ASSERT(t); return *t; }
    constexpr T* operator->() noexcept { LIBSHIT_ASSERT(t); return t; }
    constexpr const T* operator->() const noexcept { LIBSHIT_ASSERT(t); return t; }

    constexpr T& operator[](Index i) noexcept
    { LIBSHIT_ASSERT(t); return t[static_cast<IndexT>(i)]; }
    constexpr const T& operator[](Index i) const noexcept
    { LIBSHIT_ASSERT(t); return t[static_cast<IndexT>(i)]; }

    constexpr StrongPointer& operator++() { ++t; return *this; }
    constexpr StrongPointer operator++(int) { return {t++}; }
    constexpr StrongPointer& operator--() { --t; return *this; }
    constexpr StrongPointer operator--(int) { return {t--}; }

#define LIBSHIT_OP(op)                                      \
    constexpr StrongPointer operator op(Diff d) const noexcept \
    { return t op static_cast<DiffT>(d); }                     \
    constexpr StrongPointer& operator op##=(Diff d) noexcept   \
    { t op##= static_cast<DiffT>(d); return *this; }           \
    constexpr friend StrongPointer operator op(                \
      Diff d, StrongPointer o) noexcept                        \
    { return static_cast<DiffT>(d) op o.t; }
    LIBSHIT_OP(+) LIBSHIT_OP(-)
#undef LIBSHIT_OP
    constexpr Diff operator-(StrongPointer o) const noexcept
    { return Diff(t - o.t); }

    Libshit::Span<T> ToSpan(Index size) noexcept
    {
      LIBSHIT_ASSERT(!size || t);
      return {t, static_cast<IndexT>(size)};
    }

    Libshit::Span<const T> ToSpan(Index size) const noexcept
    {
      LIBSHIT_ASSERT(!size || t);
      return {t, static_cast<IndexT>(size)};
    }

    // cast helpers
#define LIBSHIT_CAST(Camel, snake)                                    \
    template <typename U>                                             \
    constexpr StrongPointer<U, Index> Camel##PtrCast() noexcept       \
    { return StrongPointer<U, Index>(snake##_cast<U*>(t)); }          \
    template <typename U>                                             \
    constexpr StrongPointer<U, Index> Camel##PtrCast() const noexcept \
    { return StrongPointer<U, Index>(snake##_cast<U*>(t)); }
    LIBSHIT_CAST(Static, static)
    LIBSHIT_CAST(Reinterpret, reinterpret)
    LIBSHIT_CAST(Const, const)
    LIBSHIT_CAST(Dynamic, dynamic)
#undef LIBSHIT_CAST

    template <typename U>
    constexpr StrongPointer<T, U> IndexCast() noexcept
    { return StrongPointer<T, U>(t); }
    template <typename U>
    constexpr StrongPointer<const T, U> IndexCast() const noexcept
    { return StrongPointer<T, U>(t); }

    // cmp
    constexpr explicit operator bool() const noexcept { return t; }
#define LIBSHIT_OP(op)                                      \
    constexpr bool operator op(StrongPointer o) const noexcept \
    { return t op o.t; }
    LIBSHIT_OP_CMP_GEN(LIBSHIT_OP)
#undef LIBSHIT_OP

  private:
    T* t;
  };

#undef LIBSHIT_OP_CMP_GEN

  template <typename T, typename Index, typename Allocator = std::allocator<T>>
  struct StrongAllocator : private Allocator
  {
    using pointer = StrongPointer<T, Index>;
    using const_pointer = StrongPointer<const T, Index>;
    using void_pointer = StrongPointer<void, Index>;
    using difference_type = typename StrongPointer<T, Index>::difference_type;
    using size_type = Index;
    using value_type = T;

    using propagate_on_container_copy_assignment =
      typename std::allocator_traits<Allocator>::propagate_on_container_copy_assignment;
    using propagate_on_container_move_assignment =
      typename std::allocator_traits<Allocator>::propagate_on_container_move_assignment;
    using propagate_on_container_swap =
      typename std::allocator_traits<Allocator>::propagate_on_container_swap;
    using is_always_equal =
      typename std::allocator_traits<Allocator>::is_always_equal;

    pointer allocate(Index n)
    { return Allocator::allocate(n.template Get<Index>()); }
    void deallocate(pointer ptr, Index n)
    { Allocator::deallocate(ptr.Get(), n.template Get<Index>()); }
    Index max_size() const noexcept
    {
      return Index(std::min(
        std::numeric_limits<typename Index::Type>::max() / sizeof(T),
        std::allocator_traits<Allocator>::max_size(*this)));
    }
    template <typename U, typename... Args>
    void construct(U* ptr, Args&&... args)
    {
      return std::allocator_traits<Allocator>::construct(
        *this, ptr, std::forward<Args>(args)...);
    }
    template <typename U> void destroy(U* ptr)
    { return std::allocator_traits<Allocator>::destroy(*this, ptr); }

    StrongAllocator select_on_container_copy_construction()
    {
      return std::allocator_traits<Allocator>::
        select_on_container_copy_construction(*this);
    }
  };

}

#endif
