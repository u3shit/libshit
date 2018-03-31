#ifndef UUID_FA716317_5F3D_4D93_B0D5_5DE60DD4B9BF
#define UUID_FA716317_5F3D_4D93_B0D5_5DE60DD4B9BF
#pragma once

#include <functional>

#include "assert.hpp"

namespace Libshit
{
  // like std::function, but only movable
  template <typename T> class Function;
  template <typename Ret, typename... Args>
  class Function<Ret (Args...)>
  {
  public:
    using result_type = Ret;

    Function() noexcept = default;
    Function(Function&& o) noexcept = default;
    template <typename T> Function(T t)
      : fun{new FunImpl<T>{std::move(t)}} {}

    Function& operator=(Function&& o) noexcept = default;

    explicit operator bool() const noexcept { return static_cast<bool>(fun); }
    Ret operator()(Args... args) const
    {
      LIBSHIT_ASSERT_MSG(fun, "Called empty Function");
      return fun->Call(
        const_cast<FunBase*>(fun.get()), std::forward<Args>(args)...);
    }

  private:
    struct FunBase
    {
      Ret (*Call)(FunBase* thiz, Args... args);
      void (*Delete)(FunBase* thiz);
    };

    template <typename T>
    struct FunImpl : FunBase
    {
      FunImpl(T t)
        : FunBase{
            [](FunBase* thiz, Args... args)
            {
              return std::invoke(static_cast<FunImpl*>(thiz)->t,
                                 std::forward<Args>(args)...);
            },
            [](FunBase* thiz) { delete static_cast<FunImpl*>(thiz); }},
          t{std::move(t)} {}
      T t;
    };

    struct FunDelete
    {
      void operator()(FunBase* fun) { fun->Delete(fun); }
    };
    std::unique_ptr<FunBase, FunDelete> fun;
  };
}

#endif
