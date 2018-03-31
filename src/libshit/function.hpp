#ifndef UUID_FA716317_5F3D_4D93_B0D5_5DE60DD4B9BF
#define UUID_FA716317_5F3D_4D93_B0D5_5DE60DD4B9BF
#pragma once

#include <functional>

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

    explicit operator bool() const noexcept { return fun; }
    Ret operator()(Args... args)
    { return fun->Call(std::forward<Args>(args)...); }

  private:
    struct FunBase
    {
      virtual ~FunBase() = default;
      virtual Ret Call(Args... args) = 0;
    };

    template <typename T>
    struct FunImpl : FunBase
    {
      FunImpl(T t) : t{std::move(t)} {}
      Ret Call(Args... args) override
      { return std::invoke(t, std::forward<Args>(args)...); }
      T t;
    };

    std::unique_ptr<FunBase> fun;
  };
}

#endif
