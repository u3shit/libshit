#ifndef GUARD_ILL_NATUREDLY_UNSCUFFED_MANY_WORLDS_INTERPRETATION_GRECIANIZES_1359
#define GUARD_ILL_NATUREDLY_UNSCUFFED_MANY_WORLDS_INTERPRETATION_GRECIANIZES_1359
#pragma once

#include "libshit/assert.hpp"

#include <algorithm>
#include <type_traits>

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
    using raw_t = std::remove_reference_t<T>;
    LIBSHIT_ASSERT_MSG(
      dynamic_cast<raw_t*>(&ref) == static_cast<raw_t*>(&ref), "U is not T");

    return static_cast<T>(ref);
  }

  template <typename T>
  struct AddConst { using Type = const T; };
  template <typename T>
  struct AddConst<T*> { using Type = typename AddConst<T>::Type* const; };

  template <typename T>
  inline T implicit_const_cast(typename AddConst<T>::Type x)
  { return const_cast<T>(x); }


  template <typename T>
  class Key
  {
    friend T;
    Key() noexcept {}
  };

  template <typename... Args> struct Overloaded : Args...
  { using Args::operator()...; };
  template <typename... Args> Overloaded(Args...) -> Overloaded<Args...>;

  /**
   * `std::move` alternative that errors on const refs (instead of silently
   * producing `const T&&`, which will not move).
   */
  template <typename T>
  constexpr std::remove_const_t<std::remove_reference_t<T>>&&
  Move(T&& t) noexcept
  { return static_cast<std::remove_const_t<std::remove_reference_t<T>>&&>(t); }

  template <typename T>
  constexpr T&& Move(const T&) noexcept = delete;

  /**
   * Execute a function when the variable is destroyed.
   * Designed as a simpler, macro less, C++17 alternate to
   * [http://www.boost.org/doc/libs/1_57_0/libs/scope_exit/doc/html/index.html](Boost.ScopeExit),
   * this allows you to execute arbitrary code when a variable goes out of
   * scope. It can be used when writing a RAII class to manage some resource
   * would be overkill.
   * @par Usage
   * @code{.cpp}
   * {
   *     auto something = InitSomething()
   *     AtScopeExit x([&]() { DeinitSomething(something); });
   *
   *     DoSomething(something);
   *     // ...
   * } // DeinitSomething (or earlier in case of an exception)
   * @endcode
   * @warning The function you give should be `noexcept`, because it'll executed
   *     in a destructor, and destructors [shouldn't throw during stack
   *     unwinding](https://isocpp.org/wiki/faq/exceptions#dtors-shouldnt-throw).
   */
  template <typename T>
  class AtScopeExit
  {
    const T func;
  public:
    explicit AtScopeExit(const T& func)
      noexcept(std::is_nothrow_copy_constructible_v<T>) : func(func) {}
    AtScopeExit(const AtScopeExit&) = delete;
    void operator=(const AtScopeExit&) = delete;

    ~AtScopeExit() noexcept(noexcept(func())) { func(); }
  };

  /// Conditional AtScopeExit, can be disabled.
  template <typename T>
  class AtScopeExitC
  {
    T func;
    bool enabled;
  public:
    explicit AtScopeExitC(const T& func, bool enabled = true)
      noexcept(std::is_nothrow_copy_constructible_v<T>)
      : func(func), enabled{enabled} {}
    AtScopeExitC(AtScopeExitC&& o)
      noexcept(std::is_nothrow_move_constructible_v<T>)
      : func(Move(o.func)), enabled{o.enabled} { o.enabled = false; }
    AtScopeExitC& operator=(AtScopeExitC o)
      noexcept(std::is_nothrow_swappable_v<T>)
    {
      using std::swap;
      swap(func, o.func);
      enabled = o.enabled;
      return *this;
    }

    void Enable() noexcept { enabled = true; }
    void Disable() noexcept { enabled = false; }
    bool IsEnabled() const noexcept { return enabled; }

    /// Execute the function if it wasn't yet
    void Fire() noexcept
    {
      if (enabled) func();
      enabled = false;
    }

    ~AtScopeExitC() noexcept(noexcept(func())) { if (enabled) func(); }
  };
}

#endif
