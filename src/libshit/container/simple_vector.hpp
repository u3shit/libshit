#ifndef GUARD_UNTASTEFULLY_PORRIDGY_ROOD_TREE_FITS_THE_BILL_2014
#define GUARD_UNTASTEFULLY_PORRIDGY_ROOD_TREE_FITS_THE_BILL_2014
#pragma once

#include "libshit/assert.hpp"
#include "libshit/except.hpp"
#include "libshit/utils.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory> // IWYU pragma: export
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Libshit
{
#if __has_feature(address_sanitizer)
  extern "C" void __sanitizer_annotate_contiguous_container(
    const void*, const void*, const void*, const void*);
#endif

  /**
   * An incomplete std::vector replacement using 1.5 as a growth factor. Unlike
   * stl implementations, this doesn't choke if size_type/difference_type is
   * only explicit constructible from ints, so it's usable with StrongAllocator.
   */
  template <typename T, typename Allocator = std::allocator<T>>
  class SimpleVector : private Allocator
  {
  private:
    using AllocTraits = std::allocator_traits<Allocator>;
    template <typename It>
    using ItTag = typename std::iterator_traits<It>::iterator_category;
    constexpr static inline auto ALIGN =
      std::max(std::size_t(1), (2*sizeof(void*)) / sizeof(T));

  public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = typename AllocTraits::size_type;
    using difference_type = typename AllocTraits::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename AllocTraits::pointer;
    using const_pointer = typename AllocTraits::const_pointer;

    using iterator = pointer;
    using const_iterator = const_pointer;

    // construct, destruct
    constexpr SimpleVector() noexcept(noexcept(Allocator())) = default;
    constexpr explicit SimpleVector(const Allocator& alloc) noexcept
      : Allocator(alloc) {}
    SimpleVector(size_type count, const T& value,
           const Allocator& alloc = Allocator())
      : Allocator(alloc) { resize(count, value); }
    explicit SimpleVector(size_type count, const Allocator& alloc = Allocator())
      : Allocator(alloc) { resize(count); }
    SimpleVector(const SimpleVector& o)
      : SimpleVector(o, AllocTraits::select_on_container_copy_construction(o)) {}
    SimpleVector(const SimpleVector& o, const Allocator& alloc)
      : Allocator(alloc) { assign(o.begin_ptr, o.end_ptr); }
    SimpleVector(SimpleVector&& o) noexcept
      : Allocator(Libshit::Move(o)), begin_ptr(o.begin_ptr),
        end_ptr(o.end_ptr), capacity_ptr(o.capacity_ptr)
    { o.begin_ptr = o.end_ptr = o.capacity_ptr = nullptr; }
    // missing: SimpleVector(SimpleVector&& o, const Allocator& alloc);
    SimpleVector(
      std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : Allocator{alloc} { assign(init); }

    template <typename InputIt>
    SimpleVector(
      InputIt beg_it, InputIt end_it, const Allocator& alloc = Allocator())
      : Allocator(alloc) { assign(beg_it, end_it); }

    ~SimpleVector() noexcept { reset(); }

    // assign
    SimpleVector& operator=(const SimpleVector& o)
    {
      if (this != &o)
      {
        if (AllocTraits::propagate_on_container_copy_assignment::value)
        {
          reset();
          static_cast<Allocator&>(*this) = o;
        }
        assign(o.begin_ptr, o.end_ptr);
      }
      return *this;
    }
    SimpleVector& operator=(SimpleVector&& o)
    {
      if (this != &o)
      {
        reset();
        static_assert(
          AllocTraits::propagate_on_container_move_assignment::value ||
          AllocTraits::is_always_equal::value); // todo
        if (AllocTraits::propagate_on_container_move_assignment::value)
          static_cast<Allocator&>(*this) = o;
        //else if (get_allocator() != o.get_allocator())

        using std::swap;
        swap(begin_ptr, o.begin_ptr);
        swap(end_ptr, o.end_ptr);
        swap(capacity_ptr, o.capacity_ptr);
      }
      return *this;
    }
    SimpleVector& operator=(std::initializer_list<T> list)
    { assign(list); return *this; }

    void assign(size_type n, const T& value)
    {
      if (n > capacity_ptr - begin_ptr) { reset(); resize_capacity(n); }
      auto p = begin_ptr, assign_until = end_ptr;
      for (; n && p != assign_until; ++p, --n) *p = value;
      if (!n) clear_to_end(p);
      for (; n; --n)
      {
        asan_annotate(end_ptr, 0, end_ptr, 1);
        try
        {
          AllocTraits::construct(
            static_cast<Allocator&>(*this), std::addressof(*p), value);
        }
        catch (...)
        {
          asan_annotate(end_ptr, 1, end_ptr, 0);
          throw;
        }
        end_ptr = ++p; // basic exception guarantee
      }
    }

    template <typename InputIt>
    void assign(InputIt beg_it, InputIt end_it)
    {
      if constexpr (std::is_base_of_v<std::forward_iterator_tag, ItTag<InputIt>>)
      {
        auto n = std::distance(beg_it, end_it);
        if (n > capacity_ptr - begin_ptr) { reset(); resize_capacity(n); }

        auto p = begin_ptr, assign_until = end_ptr;
        for (; n && p != assign_until; ++p, --n) *p = *beg_it++;
        if (!n) clear_to_end(p);
        for (; n; --n)
        {
          asan_annotate(end_ptr, 0, end_ptr, 1);
          try
          {
            AllocTraits::construct(
              static_cast<Allocator&>(*this), std::addressof(*p), *beg_it++);
          }
          catch (...)
          {
            asan_annotate(end_ptr, 1, end_ptr, 0);
            throw;
          }
          end_ptr = ++p; // basic exception guarantee
        }
        LIBSHIT_ASSERT(beg_it == end_it);
      }
      else
      {
        auto p = begin_ptr;
        for (; p != end_ptr && beg_it != end_it; ++p, ++beg_it)
          *p = *begin_ptr;
        if (beg_it == end_it) clear_to_end(p);
        for (; beg_it != end_it; ++beg_it) push_back(*beg_it);
      }
    }

    void assign(std::initializer_list<T> list)
    { assign(list.begin(), list.end()); }

    constexpr allocator_type get_allocator() const { return *this; }

    // access
    reference at(size_type pos)
    {
      if (pos >= end_ptr - begin_ptr)
        LIBSHIT_THROW(std::out_of_range, "Vector::at");
      return begin_ptr[pos];
    }
    const_reference at(size_type pos) const
    {
      if (pos >= end_ptr - begin_ptr)
        LIBSHIT_THROW(std::out_of_range, "Vector::at");
      return begin_ptr[pos];
    }
    reference operator[](size_type pos) noexcept
    {
      LIBSHIT_ASSERT(pos < end_ptr - begin_ptr);
      return begin_ptr[pos];
    }
    const_reference operator[](size_type pos) const noexcept
    {
      LIBSHIT_ASSERT(pos < end_ptr - begin_ptr);
      return begin_ptr[pos];
    }

    reference front() noexcept
    {
      LIBSHIT_ASSERT(begin_ptr != end_ptr);
      return *begin_ptr;
    }
    const_reference front() const noexcept
    {
      LIBSHIT_ASSERT(begin_ptr != end_ptr);
      return *begin_ptr;
    }
    reference back() noexcept
    {
      LIBSHIT_ASSERT(begin_ptr != end_ptr);
      auto copy = end_ptr;
      return *--end_ptr;
    }
    const_reference back() const noexcept
    {
      LIBSHIT_ASSERT(begin_ptr != end_ptr);
      auto copy = end_ptr;
      return *--end_ptr;
    }
    T* data() noexcept
    { return begin_ptr == nullptr ? nullptr : std::addressof(*begin_ptr); }
    const T* data() const noexcept
    { return begin_ptr == nullptr ? nullptr : std::addressof(*begin_ptr); }
    // extension -- return allocator's pointer
    pointer pdata() noexcept { return begin_ptr; }
    const_pointer pdata() const noexcept { return begin_ptr; }

    // iterators
    iterator begin() noexcept { return begin_ptr; }
    const_iterator begin() const noexcept { return begin_ptr; }
    const_iterator cbegin() const noexcept { return begin_ptr; }
    iterator end() noexcept { return end_ptr; }
    const_iterator end() const noexcept { return end_ptr; }
    const_iterator cend() const noexcept { return end_ptr; }
    // todo reverse iterators

    // raw (pointer) iterators
    T* wbegin()
    { return begin_ptr == nullptr ? nullptr : std::addressof(*begin_ptr); }
    const T* wbegin() const
    { return begin_ptr == nullptr ? nullptr : std::addressof(*begin_ptr); }
    T* wend()
    { return end_ptr == nullptr ? nullptr : std::addressof(*end_ptr); }
    const T* wend() const
    { return end_ptr == nullptr ? nullptr : std::addressof(*end_ptr); }


    // capacity
    constexpr bool empty() const noexcept { return begin_ptr == end_ptr; }
    constexpr size_type size() const noexcept { return end_ptr - begin_ptr; }
    constexpr size_type max_size() const noexcept
    {
      return AllocTraits::max_size(static_cast<const Allocator&>(*this)) -
        ALIGN;
    }
    void reserve(size_type n)
    {
      if (n <= capacity_ptr - begin_ptr) return;
      resize_capacity(n);
    }
    constexpr size_type capacity() const noexcept
    { return capacity_ptr - begin_ptr; }
    void shrink_to_fit()
    {
      if (begin_ptr == end_ptr) reset();
      else resize_capacity(end_ptr - begin_ptr);
    }
    // extension -- clear & shrink_to_fit in one
    constexpr void reset() noexcept
    {
      for (auto p = begin_ptr; p != end_ptr; ++p)
        AllocTraits::destroy(static_cast<Allocator&>(*this), std::addressof(*p));
      asan_annotate(end_ptr, 0, capacity_ptr, 0);
      AllocTraits::deallocate(
        static_cast<Allocator&>(*this), begin_ptr, capacity_ptr - begin_ptr);
      begin_ptr = end_ptr = capacity_ptr = nullptr;
    }

    // modifiers
    void clear() noexcept { clear_to_end(begin_ptr); }
    // todo insert
    // todo emplace

    // vector::erase has the basic exception safety guarantee
    // https://stackoverflow.com/a/28139567
    iterator erase(const_iterator it)
    {
      LIBSHIT_ASSERT(it >= begin_ptr && it < end_ptr);
      std::move(it+1, end(), it);
      pop_back();
      return it;
    }

    iterator erase(const_iterator first, const_iterator last)
    {
      LIBSHIT_ASSERT(begin_ptr <= first && first <= last && last <= end_ptr);
      std::move(last, end_ptr, first);
      resize(end_ptr - begin_ptr - (last-first));
      return first;
    }

    // O(1) erase that doesn't keep order: moves the last item into the erased
    iterator unordered_erase(const_iterator it)
      noexcept(std::is_nothrow_move_assignable_v<T>)
    {
      LIBSHIT_ASSERT(it >= begin_ptr && it < end_ptr);
      *it = Move(back());
      pop_back();
      return it;
    }

    reference push_back(const T& t) { return emplace_back(t); }
    reference push_back(T&& t) { return emplace_back(Libshit::Move(t)); }
    template <typename... Args>
    reference emplace_back(Args&&... args)
    {
      if (end_ptr == capacity_ptr)
      {
        auto max = max_size();
        if (capacity_ptr - begin_ptr >= max)
          LIBSHIT_THROW(std::length_error, "Vector::emplace_back");
        auto size = end_ptr - begin_ptr;
        if (size >= max / 3) // this will result in a sudden 3x growth factor...
          resize_capacity(max);
        else
          resize_capacity(std::max(difference_type(4), size * 3 / 2));
      }
      asan_annotate(end_ptr, 0, end_ptr, 1);
      try
      {
        AllocTraits::construct(
          static_cast<Allocator&>(*this), std::addressof(*end_ptr),
          std::forward<Args>(args)...);
      }
      catch (...)
      {
        asan_annotate(end_ptr, 1, end_ptr, 0);
        throw;
      }
      return *end_ptr++;
    }
    void pop_back() noexcept
    {
      LIBSHIT_ASSERT(begin_ptr != end_ptr);
      auto p = end_ptr;
      AllocTraits::destroy(static_cast<Allocator&>(*this), std::addressof(*--p));
      asan_annotate(end_ptr, 0, p, 0);
      end_ptr = p;
    }
    template <typename... Args> // extension
    void resize(size_type n, const Args&... args)
    {
      reserve(n);
      auto new_end = begin_ptr + n;
      auto p = end_ptr;
      if (p < new_end)
        asan_annotate(end_ptr, 0, new_end, 0);
      try
      {
        for (; p < new_end; ++p)
          AllocTraits::construct(
            static_cast<Allocator&>(*this), std::addressof(*p), args...);
      }
      catch (...)
      {
        while (p != end_ptr)
          AllocTraits::destroy(
            static_cast<Allocator&>(*this), std::addressof(*--p));
        asan_annotate(new_end, 0, end_ptr, 0);
        throw;
      }
      for (auto p = new_end; p < end_ptr; ++p)
        AllocTraits::destroy(static_cast<Allocator&>(*this), std::addressof(*p));
      if (new_end < end_ptr)
        asan_annotate(end_ptr, 0, new_end, 0);
      end_ptr = new_end;
    }
    void swap(SimpleVector& o) noexcept
    {
      using std::swap;
      if constexpr (AllocTraits::propagate_on_container_swap::value)
        swap(static_cast<Allocator&>(*this), static_cast<Allocator&>(o));
      else
        LIBSHIT_ASSERT(static_cast<Allocator&>(*this) == static_cast<Allocator&>(o));
      swap(begin_ptr, o.begin_ptr);
      swap(end_ptr, o.end_ptr);
      swap(capacity_ptr, o.capacity_ptr);
    }

  private:
    pointer begin_ptr = nullptr, end_ptr = nullptr, capacity_ptr = nullptr;

    void asan_annotate(
      const pointer& old_end, int old_offs, const pointer& new_end, int new_offs)
    {
#if __has_feature(address_sanitizer)
      if (begin_ptr != nullptr)
        __sanitizer_annotate_contiguous_container(
          std::addressof(*begin_ptr), std::addressof(*capacity_ptr),
          std::addressof(*old_end) + old_offs, std::addressof(*new_end) + new_offs);
#endif
    }

    void resize_capacity(size_type n)
    {
      // TODO: this is not optimal for NPOT sizes < 2*sizeof(void*)
      n = (n+(ALIGN-1))/ALIGN*ALIGN;
      if (n == capacity_ptr - begin_ptr) return;

      auto new_beg = AllocTraits::allocate(static_cast<Allocator&>(*this), n);
      auto q = new_beg;
      try
      {
        for (auto p = begin_ptr; p != end_ptr; ++p, ++q)
          AllocTraits::construct(
            static_cast<Allocator&>(*this), std::addressof(*q),
            std::move_if_noexcept(*p));
      }
      catch (...)
      {
        while (q != new_beg)
          AllocTraits::destroy(
            static_cast<Allocator&>(*this), std::addressof(*--q));
        AllocTraits::deallocate(static_cast<Allocator&>(*this), new_beg, n);
        throw;
      }

      reset();
      begin_ptr = new_beg;
      end_ptr = q;
      capacity_ptr = new_beg + n;
      asan_annotate(capacity_ptr, 0, end_ptr, 0);
    }

    void clear_to_end(pointer new_end)
    {
      for (auto p = new_end; p != end_ptr; ++p)
        AllocTraits::destroy(static_cast<Allocator&>(*this), std::addressof(*p));
      asan_annotate(end_ptr, 0, new_end, 0);
      end_ptr = new_end;
    }
  };

}

#endif
