#ifndef UUID_FA716317_5F3D_4D93_B0D5_5DE60DD4B9BF
#define UUID_FA716317_5F3D_4D93_B0D5_5DE60DD4B9BF
#pragma once

#include "libshit/assert.hpp"
#include "libshit/utils.hpp"

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace Libshit
{
  template <auto Fun>
  struct Func
  {
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const
      noexcept(noexcept(std::invoke(Fun, std::forward<Args>(args)...)))
    { return std::invoke(Fun, std::forward<Args>(args)...); }
  };

  /// Turn function ptr/member ptrs into stateless function objects
  template <auto Fun>
  constexpr const inline Func<Fun> FUNC;

  /**
   * Like std::function, but only movable. Implements const qualified signature
   * from P0045R1, e.g. `Function<void (int) const>`.
   * @see Function
   */
  template <size_t Size, bool IsConst, typename Ret, typename... Args>
  class FunctionImpl final
  {
  public:
    using result_type = Ret;

    FunctionImpl() noexcept = default;
    FunctionImpl(FunctionImpl&& o) noexcept : vptr{o.vptr}
    { if (vptr) vptr->Move(&buf, &o.buf); }

    FunctionImpl& operator=(FunctionImpl&& o) noexcept
    {
      reset();
      vptr = o.vptr;
      if (vptr) vptr->Move(&buf, &o.buf);
      return *this;
    }
    ~FunctionImpl() noexcept { reset(); }

    template <typename T>
    FunctionImpl(T t) : vptr{&static_vptr<T>}
    {
      LIBSHIT_STATIC_WARNING(!std::is_pointer_v<T>, use_FUNC_if_possible);
      VptrImpl<T>::Construct(&buf, Move(t));
    }

    template <typename T, typename... CArgs>
    FunctionImpl(std::in_place_type_t<T>, CArgs&&... args)
      : vptr{&static_vptr<T>}
    { VptrImpl<T>::Construct(&buf, std::forward<CArgs>(args)...); }

    void reset() noexcept
    {
      if (!vptr) return;
      vptr->Delete(&buf);
      vptr = nullptr;
    }

    explicit operator bool() const noexcept { return static_cast<bool>(vptr); }

    template <bool C = IsConst, typename = std::enable_if_t<C>>
    Ret operator()(Args... args) const
    {
      LIBSHIT_ASSERT_MSG(vptr, "Called empty Function");
      return vptr->Call(&buf, std::forward<Args>(args)...);
    }

    template <bool C = IsConst, typename = std::enable_if_t<!C>>
    Ret operator()(Args... args)
    {
      LIBSHIT_ASSERT_MSG(vptr, "Called empty Function");
      return vptr->Call(&buf, std::forward<Args>(args)...);
    }

  private:
    using VoidPtr = std::conditional_t<IsConst, const void*, void*>;

    struct Vptr
    {
      Ret (*Call)(VoidPtr buf, Args&&... args);
      void (*Delete)(void* buf) noexcept;
      void (*Move)(void* new_buf, void* old_buf) noexcept;
    };

    template <typename T, typename = void>
    struct VptrImpl : Vptr
    {
      static_assert(sizeof(T*) <= Size);
      using TPPtr = std::conditional_t<IsConst, T const* const*, T**>;

      constexpr VptrImpl() : Vptr{
        [](VoidPtr buf, Args&&... args) -> Ret
        {
          auto t = *static_cast<TPPtr>(buf);
          LIBSHIT_ASSERT_MSG(t, "Called moved out Function");
          return std::invoke(*t, std::forward<Args>(args)...);
        },
        [](void* buf) noexcept { delete *static_cast<T**>(buf); },
        [](void* new_buf, void* old_buf) noexcept
        {
          T** new_t = static_cast<T**>(new_buf);
          T** old_t = static_cast<T**>(old_buf);
          *new_t = *old_t;
          *old_t = nullptr;
        }} {}

      template <typename... CArgs>
      static void Construct(void* buf, CArgs&&... args)
      { *static_cast<T**>(buf) = new T(std::forward<CArgs>(args)...); }
    };

    template <typename T>
    /* todo check align */
    struct VptrImpl<T, std::enable_if_t<
      sizeof(T) <= Size && std::is_nothrow_move_constructible_v<T>>> : Vptr
    {
      using TPtr = std::conditional_t<IsConst, const T*, T*>;
      constexpr VptrImpl() : Vptr{
        [](VoidPtr buf, Args&&... args) -> Ret
        {
          return std::invoke(
            *static_cast<TPtr>(buf), std::forward<Args>(args)...);
        },
        [](void* buf) noexcept { static_cast<T*>(buf)->~T(); },
        [](void* new_buf, void* old_buf) noexcept
        { new (new_buf) T(Move(*static_cast<T*>(old_buf))); }} {}

      template <typename... CArgs>
      static void Construct(void* buf, CArgs&&... args)
      { new (buf) T(std::forward<CArgs>(args)...); }
    };

    template <typename T> constexpr inline static VptrImpl<T> static_vptr{};

    std::aligned_storage_t<Size> buf;
    const Vptr* vptr = nullptr;
  };

  template <typename T, size_t Size = 3*sizeof(void*)> struct FunctionT;

  template <typename Ret, typename... Args, size_t Size>
  struct FunctionT<Ret (Args...), Size>
  { using type = FunctionImpl<Size, false, Ret, Args...>; };

  template <typename Ret, typename... Args, size_t Size>
  struct FunctionT<Ret (Args...) const, Size>
  { using type = FunctionImpl<Size, true, Ret, Args...>; };

  /**
   * Like std::function, but only movable. Implements const qualified signature
   * from P0045R1, e.g. `Function<void (int) const>`.
   * @see Libshit::FunctionImpl
   */
  template <typename T, size_t Size = 3*sizeof(void*)>
  using Function = typename FunctionT<T, Size>::type;

}

#endif
