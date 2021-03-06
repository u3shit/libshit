#ifndef GUARD_SLEEKLY_TROCHLEAR_REPENETRATION_BARNEYS_1688
#define GUARD_SLEEKLY_TROCHLEAR_REPENETRATION_BARNEYS_1688
#pragma once

#include "libshit/assert.hpp"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>

namespace Libshit
{

  namespace Detail
  {
    template <typename Ptr, typename DiffType, typename = void>
    struct GetSizeType { using Type = std::make_unsigned_t<DiffType>; };

    template <typename Ptr, typename DiffType>
    struct GetSizeType<Ptr, DiffType, std::void_t<typename Ptr::size_type>>
    { using Type = typename Ptr::size_type; };
  }

  /// Simple std::span. No static extent support.
  template <typename T, typename Pointer = T*>
  class Span
  {
    using PtrTraits = std::pointer_traits<Pointer>;
    static_assert(std::is_same_v<T, typename PtrTraits::element_type>);

  public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using difference_type = typename PtrTraits::difference_type;
    using index_type = typename Detail::GetSizeType<Pointer, difference_type>::Type;
    using pointer = Pointer;
    using const_pointer = typename PtrTraits::template rebind<const T>;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    constexpr Span() noexcept = default;
    constexpr Span(pointer ptr, index_type size) noexcept
      : ptr{ptr}, psize{size} {}
    constexpr Span(pointer begin, pointer end) noexcept
      : ptr{begin}, psize(end-begin) {}

    template <auto N>
    constexpr Span(element_type (&ary)[N]) noexcept
      : ptr{ary}, psize{N} {}

    // nonstandard: simplified container->span conversion
    template <typename Container>
    constexpr Span(Container&& c)
      noexcept(noexcept(std::data(c)) && noexcept(std::size(c)))
      : ptr{std::data(c)}, psize{std::size(c)} {}
    template <typename Container>
    constexpr Span(const Container& c)
      noexcept(noexcept(std::data(c)) && noexcept(std::size(c)))
      : ptr{std::data(c)}, psize{std::size(c)} {}

    template <typename U>
    constexpr Span(const Span<U>& other) noexcept
      : ptr{other.data()}, psize{other.size()} {}

    // extension: from initializer_list
    constexpr Span(std::initializer_list<T> list) noexcept
      : ptr{list.begin()}, psize{list.size()} {}

    constexpr Span(const Span& o) noexcept = default;
    constexpr Span& operator=(const Span& o) noexcept = default;


    constexpr iterator begin() const noexcept { return ptr; }
    constexpr const_iterator cbegin() const noexcept { return ptr; }
    constexpr iterator end() const noexcept { return ptr+psize; }
    constexpr const_iterator cend() const noexcept { return ptr+psize; }

    constexpr reverse_iterator rbegin() const noexcept { return end(); }
    constexpr const_reverse_iterator crbegin() const noexcept { return cend(); }
    constexpr reverse_iterator rend() const noexcept { return begin(); }
    constexpr const_reverse_iterator crend() const noexcept { return cbegin(); }

    // extension: raw (pointer) iterators
    T* wbegin()
    { return ptr == nullptr ? nullptr : std::addressof(*ptr); }
    const T* wbegin() const
    { return ptr == nullptr ? nullptr : std::addressof(*ptr); }
    T* wend() { return ptr == nullptr ? nullptr : std::addressof(ptr[psize]); }
    const T* wend() const
    { return ptr == nullptr ? nullptr : std::addressof(ptr[psize]); }


    constexpr reference front() const noexcept
    { LIBSHIT_ASSERT(psize); return *ptr; }
    constexpr reference back() const noexcept
    { LIBSHIT_ASSERT(psize); return ptr[psize-index_type(1)]; }

    constexpr reference operator[](index_type i) noexcept
    { LIBSHIT_ASSERT(i < psize); return ptr[i]; }
    constexpr const_reference operator[](index_type i) const noexcept
    { LIBSHIT_ASSERT(i < psize); return ptr[i]; }
    constexpr pointer data() const noexcept { return ptr; }

    constexpr index_type size() const noexcept { return psize; }
    constexpr index_type size_bytes() const noexcept
    { return psize * sizeof(element_type); }
    constexpr bool empty() const noexcept { return !psize; }


    constexpr Span first(index_type n) const noexcept
    { LIBSHIT_ASSERT(n <= psize); return {ptr, n}; }
    constexpr Span last(index_type n) const noexcept
    { LIBSHIT_ASSERT(n <= psize); return {ptr+(psize-n), n}; }
    constexpr Span subspan(index_type off, index_type n) const noexcept
    {
      LIBSHIT_ASSERT(n <= psize);
      LIBSHIT_ASSERT(off+n <= psize);
      return {ptr+off, n};
    }

  private:
    pointer ptr = nullptr;
    index_type psize{0};
  };

  template <typename T, std::size_t N>
  Span(T (&)[N]) -> Span<T>;

  template <typename T>
  Span(T&) -> Span<typename T::value_type>;
  template <typename T>
  Span(const T&) -> Span<const typename T::value_type>;
}

#endif
