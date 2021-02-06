#ifndef UUID_4999021A_F41A_400B_A951_CDE022AF7331
#define UUID_4999021A_F41A_400B_A951_CDE022AF7331
#pragma once

#include "libshit/utils.hpp"

#include "libshit/platform.hpp"

#include <boost/config.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <errno.h>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#define LIBSHIT_EXCEPTION_WITH_SOURCE_LOCATION LIBSHIT_IS_DEBUG

#if LIBSHIT_EXCEPTION_WITH_SOURCE_LOCATION
#  include "libshit/file.hpp" // IWYU pragma: export
#endif

namespace Libshit
{
  struct ExceptionInfo;
  void intrusive_ptr_add_ref(ExceptionInfo* ptr);
  void intrusive_ptr_release(ExceptionInfo* ptr);

  template <typename T>
  decltype(std::declval<std::ostream>() << std::declval<T>(), std::string{})
  inline ToString(const T& t)
  {
    std::stringstream ss;
    ss << t;
    return ss.str();
  }

  inline std::string ToString(std::string s) noexcept { return s; }
  inline std::string ToString(const char* s) { return s; }

#define LIBSHIT_GEN(type) \
  inline std::string ToString(type t) { return std::to_string(t); }
  LIBSHIT_GEN(char)  LIBSHIT_GEN(unsigned char) LIBSHIT_GEN(signed char)
  LIBSHIT_GEN(short) LIBSHIT_GEN(unsigned short)
  LIBSHIT_GEN(int)   LIBSHIT_GEN(unsigned int)
  LIBSHIT_GEN(long)  LIBSHIT_GEN(unsigned long)
  LIBSHIT_GEN(long long) LIBSHIT_GEN(unsigned long long)
  LIBSHIT_GEN(float) LIBSHIT_GEN(double) LIBSHIT_GEN(long double)
#undef LIBSHIT_GEN

  class Exception
  {
  public:
    virtual ~Exception() noexcept = default;

    void AddInfo(std::string key, std::string value);
    void AddLocation(const char* file, unsigned line, const char* func);

    void PrintDesc(std::ostream& os, bool color) const;

    template <typename T>
    void AddInfo(std::string key, const T& value)
    { AddInfo(Move(key), ToString(value)); }

    std::string operator[](const std::string& s) const;

  private:
    void EnsureInfo();
    boost::intrusive_ptr<ExceptionInfo> info;
  };

  template <typename T>
  struct MakeExceptionClass : virtual T, virtual Exception
  {
    using T::T;
    MakeExceptionClass(const T& t) noexcept(std::is_nothrow_copy_constructible_v<T>)
      : T(t) {}
    MakeExceptionClass(T&& t) noexcept(std::is_nothrow_move_constructible_v<T>)
      : T(Move(t)) {}
  };

  template <typename T, typename = void> struct EnableErrorInfoT
  { using Type = MakeExceptionClass<T>; };
  template <typename T>
  struct EnableErrorInfoT<T, std::enable_if_t<std::is_base_of_v<Exception, T>>>
  { using Type = T; };

  template <typename T>
  using EnableErrorInfo = typename EnableErrorInfoT<T>::Type;


  template <typename T> inline T&& AddInfos(T&& t) noexcept
  { return std::forward<T>(t); }

  template <typename T, typename Key, typename Value, typename... Rest>
  inline T&& AddInfos(T&& except, Key&& key, Value&& value, Rest&&... rest)
  {
    except.AddInfo(std::forward<Key>(key), std::forward<Value>(value));
    return AddInfos(std::forward<T>(except), std::forward<Rest>(rest)...);
  }

  template <typename T, typename What, typename... Args>
  inline EnableErrorInfo<T> GetException(
    const char* file, unsigned line, const char* func, What&& what,
    Args&&... args)
  {
    EnableErrorInfo<T> ret{std::forward<What>(what)};
    ret.AddLocation(file, line, func);
    AddInfos(ret, std::forward<Args>(args)...);
    return ret;
  }

  template <typename T, typename... What, typename... Args>
  inline EnableErrorInfo<T> GetException(
    const char* file, unsigned line, const char* func,
    std::tuple<What&&...> what, Args&&... args)
  {
    auto ret = std::make_from_tuple<EnableErrorInfo<T>>(what);
    ret.AddLocation(file, line, func);
    AddInfos(ret, std::forward<Args>(args)...);
    return ret;
  }

#if LIBSHIT_EXCEPTION_WITH_SOURCE_LOCATION
#  define LIBSHIT_GET_EXCEPTION(type, ...) \
  ::Libshit::GetException<type>(           \
    LIBSHIT_FILE, __LINE__, LIBSHIT_FUNCTION, __VA_ARGS__)
#else
#  define LIBSHIT_GET_EXCEPTION(type, ...) \
  ::Libshit::GetException<type>(nullptr, 0, nullptr, __VA_ARGS__)
#endif

#define LIBSHIT_THROW(type, ...) \
  (throw LIBSHIT_GET_EXCEPTION(type, __VA_ARGS__))

  BOOST_NORETURN void RethrowException();
  std::string ExceptionToString(bool color);
  std::string ExceptionToString(const Exception& e, bool color);

  struct PrintActiveException { bool color; };
  constexpr inline PrintActiveException PrintException(bool color) noexcept
  { return {color}; }
  std::ostream& operator<<(std::ostream& os, PrintActiveException e);

  struct PrintCustomException { const Exception& e; bool color; };
  constexpr inline PrintCustomException PrintException(
    const Exception& e, bool color) noexcept
  { return {e, color}; }
  std::ostream& operator<<(std::ostream& os, PrintCustomException e);

#define LIBSHIT_GEN_EXCEPTION_TYPE(name, base)     \
  struct name : base, virtual ::Libshit::Exception \
  {                                                \
    using BaseType = base;                         \
    using BaseType::BaseType;                      \
  }

  // AddInfo([&](){ ...; }, [&](auto& e) { AddInfo(e, foo...); });
  template <typename Fun, typename Info, typename ... Args>
  inline auto AddInfo(Fun fun, Info info_adder, Args&& ... args)
  {
    try
    {
      try { return std::invoke(fun, std::forward<Args>(args) ...); }
      catch (const Exception& e) { throw; }
      catch (...) { RethrowException(); }
    }
    catch (Exception& e)
    {
      info_adder(e);
      throw;
    }
  }

#define LIBSHIT_ADD_INFOS(expr, ...)               \
  try                                              \
  {                                                \
    try { expr; }                                  \
    catch (const ::Libshit::Exception&) { throw; } \
    catch (...) { ::Libshit::RethrowException(); } \
  }                                                \
  catch (::Libshit::Exception& add_infos_e)        \
  {                                                \
    ::Libshit::AddInfos(add_infos_e, __VA_ARGS__); \
    throw;                                         \
  }

  LIBSHIT_GEN_EXCEPTION_TYPE(DecodeError,    std::runtime_error);
  /// Virtual method not implemented
  LIBSHIT_GEN_EXCEPTION_TYPE(NotImplemented, std::logic_error);

#define LIBSHIT_VALIDATE_FIELD(msg, x)                                    \
  while (!(x)) LIBSHIT_THROW(Libshit::DecodeError, msg ": invalid data",  \
                             "Failed Expression", #x)

  // std::system_error is a mess (just look at this page:
  // https://stackoverflow.com/questions/28746372/system-error-categories-and-standard-system-error-codes)
  // libc++ implementation doesn't know about winapi error codes at all and
  // implementing it normally is needlessly complicated, so here's a lightweight
  // alternative to it
  LIBSHIT_GEN_EXCEPTION_TYPE(SystemError, std::runtime_error);
  struct ErrnoError : SystemError
  {
    int err_code;
    ErrnoError(int err_code);
  };

  // ...: api_function_name, other_params...
#define LIBSHIT_THROW_ERRNO(...) \
  LIBSHIT_THROW(Libshit::ErrnoError, errno, "API function", __VA_ARGS__)


#if LIBSHIT_OS_IS_WINDOWS
  struct WindowsError : SystemError
  {
    unsigned long /*DWORD*/ err_code;
    WindowsError(unsigned long err_code);
  };

#define LIBSHIT_THROW_WINERROR(...)                    \
  LIBSHIT_THROW(Libshit::WindowsError, GetLastError(), \
                "API function", __VA_ARGS__)
#endif

}

#endif
