#include "libshit/except.hpp"

#include "libshit/assert.hpp"
#include "libshit/platform.hpp"

// do not use, unless you have to: very slow (executes addr2line for *each*
// stacktrace line), flaky (has hard coded full path to addr2line)
// #define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/core/demangle.hpp>
#include <boost/stacktrace.hpp>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <memory>
#include <new>
#include <typeinfo>
#include <variant>

#define LIBSHIT_LOG_NAME "except"
#include "libshit/logger_helper.hpp"

// windows only
extern "C" void _assert(const char* msg, const char* file, unsigned line);

namespace Libshit
{
  static void PrintStacktrace(
    std::ostream& os, const boost::stacktrace::stacktrace& trace, bool color)
  {
    if (color) os << "\033[1m";
    os << "Stacktrace";
    if (color) os << "\033[22m";
    os << ":\n";
    auto flags = os.flags();
    os.width(0);
    for (std::size_t i = 0, n = trace.size(); i < n; ++i)
    {
      if (color) os << "\033[37m";
      os << std::setw(4) << std::setfill(' ') << i << ": ";
      if (color) os << "\033[36m";
      os << std::setw(sizeof(void*)*2) << std::setfill('0') << std::hex
         << reinterpret_cast<std::uintptr_t>(trace[i].address()) << std::dec;
      if (color) os << "\033[33m";
      auto name = trace[i].name();
      if (name.empty()) os << " ??";
      else os << ' ' << name;

      if (color) os << "\033[37m";
      auto line = trace[i].source_line();
      if (line)
      {
        os << " at ";
        if (color) os << "\033[32m";
        os << trace[i].source_file();
        if (color) os << "\033[37m";
        os << ':';
        if (color) os << "\033[1;37m";
        os << line;
        if (color) os << "\033[0;37m";
      }

      // uh-oh
      boost::stacktrace::detail::location_from_symbol loc(trace[i].address());
      if (!loc.empty())
      {
        os << " in ";
        if (color) os << "\033[34m";
        os << loc.name();
      }

      if (color) os << "\033[0m";
      if (i < n-1) os << '\n';
    }
    os.flags(flags);
  }

  struct ExceptionInfo
  {
    std::atomic<unsigned> refcount = 0;
    const char* file = nullptr;
    unsigned line = 0;
    const char* func = nullptr;
#if LIBSHIT_IS_DEBUG
    boost::stacktrace::stacktrace trace;
#endif
    std::multimap<std::string, std::string> map;

    ExceptionInfo() = default;
    ExceptionInfo(const ExceptionInfo& o)
      : file{o.file}, line{o.line}, func{o.func}, map{o.map} {}
  };

  void intrusive_ptr_add_ref(ExceptionInfo* ptr)
  { ++ptr->refcount; }
  void intrusive_ptr_release(ExceptionInfo* ptr)
  {
    if (--ptr->refcount == 0)
      delete ptr;
  }

  void Exception::EnsureInfo()
  {
    if (!info)
    {
      info.reset(new ExceptionInfo);
      return;
    }
    if (info->refcount != 1)
      info.reset(new ExceptionInfo{*info});
  }

  void Exception::AddInfo(std::string key, std::string value)
  {
    EnsureInfo();
    info->map.emplace(Move(key), Move(value));
  }

  void Exception::AddLocation(
    const char* file, unsigned int line, const char* func)
  {
    EnsureInfo();
    info->file = file;
    info->line = line;
    info->func = func;
  }

  void Exception::PrintDesc(std::ostream& os, bool color) const
  {
    if (!info) return;
    if (color) os << "\033[1m";
    os << "Exception";
    if (color) os << "\033[0m";
    if (info->file)
    {
      os << " at ";
      if (color) os << "\033[32m";
      os << info->file;
      if (color) os << "\033[0m";
      os << ':';
      if (color) os << "\033[1m";
      os << info->line;
      if (color) os << "\033[0m";
    }
    if (info->func)
    {
      os << " in ";
      if (color) os << "\033[33m";
      os << info->func;
      if (color) os << "\033[0m";
    }
    os << "\n";

    for (auto& e : info->map)
    {
      if (color) os << "\033[1m";
      os << e.first;
      if (color) os << "\033[22m";
      os << ": " << e.second << '\n';
    }

#if LIBSHIT_IS_DEBUG
    PrintStacktrace(os, info->trace, color);
#endif
  }

  std::string Exception::operator[](const std::string& key) const
  {
    if (!info) return {};
    auto& map = info->map;
    auto it = map.find(key);
    if (it == map.end()) return {};
    return it->second;
  }

  void RethrowException()
  {
    try { throw; }
#define RETHROW(ex)                                            \
    catch (const ex& e)                                        \
    {                                                          \
      throw AddInfos(EnableErrorInfo<ex>{e}, "Rethrown type", \
                     boost::core::demangle(typeid(e).name())); \
    }
    RETHROW(std::range_error)
    RETHROW(std::overflow_error)
    RETHROW(std::underflow_error)
    //RETHROW(std::regex_error) not used
    RETHROW(std::ios_base::failure)
    //RETRHOW(std::filesystem::filesystem_error) fs not supported in vc12
    RETHROW(std::system_error)
    RETHROW(std::runtime_error)

    //RETHROW(std::bad_optional_access) no optional in vc12

    RETHROW(std::domain_error)
    RETHROW(std::invalid_argument)
    RETHROW(std::length_error)
    RETHROW(std::out_of_range)
    RETHROW(std::logic_error)

    RETHROW(std::bad_typeid)
    RETHROW(std::bad_cast)
    RETHROW(std::bad_weak_ptr)
    RETHROW(std::bad_function_call)
    RETHROW(std::bad_alloc)
    RETHROW(std::bad_exception)
    RETHROW(std::bad_variant_access)
  }

  std::ostream& operator<<(std::ostream& os, PrintCustomException info)
  {
    auto se = dynamic_cast<const std::exception*>(&info.e);
    if (se)
    {
      if (info.color) os << "\033[1m";
      os << se->what();
      if (info.color) os << "\033[0m";
      os << "\n";
    }
    if (info.color) os << "\033[1m";
    os << "Type";
    if (info.color) os << "\033[22m";
    os << ": " << boost::core::demangle(typeid(info.e).name()) << "\n";
    info.e.PrintDesc(os, info.color);
    return os;
  }

  std::string ExceptionToString(const Exception& e, bool color)
  {
    std::stringstream ss;
    ss << PrintCustomException{e, color};
    return ss.str();
  }

  std::ostream& operator<<(std::ostream& os, PrintActiveException info)
  {
    try { throw; }
    catch (const Exception& e)
    {
      os << PrintCustomException{e, info.color};
    }
    catch (const std::exception& e)
    {
      os << boost::core::demangle(typeid(e).name()) + ": " + e.what();
    }
    catch (...)
    {
      os << "Unknown exception (run while you can)";
    }
    return os;
  }

  std::string ExceptionToString(bool color)
  {
    std::stringstream ss;
    ss << PrintActiveException{color};
    return ss.str();
  }

  void AssertFailed(
    const char* expr, const char* msg, const char* file, unsigned line,
    const char* fun)
  {
    if constexpr (LIBSHIT_OS_IS_WINDOWS)
    {
      std::stringstream ss{expr};
      if (fun) ss << "\nFunction: " << fun;
      if (msg) ss << "\nMessage: " << msg;
      ss << "\n";
      PrintStacktrace(ss, boost::stacktrace::stacktrace{}, false);
      _assert(ss.str().c_str(), file ? file : "", line);
    }
    else
    {
      auto color = Libshit::Logger::HasAnsiColor();
      auto& log = Logger::Log("assert", Logger::ERROR, file, line, fun);
      if (color) log << "\033[1m";
      log << "Assertion failed";
      if (!Logger::show_fun && fun)
      {
        if (color) log << "\033[22m";
        log << " in function ";
        if (color) log << "\033[33m";
        log << fun;
      }
      if (color) log << "\033[0m";
      log << '\n';
      if (color) log << "\033[1m";
      log << "Expression";
      if (color) log << "\033[22m";
      log << ": ";
      if (color) log << "\033[35m";
      log << expr;
      if (color) log << "\033[0m";
      log << '\n';
      if (msg)
      {
        if (color) log << "\033[1m";
        log << "Message";
        if (color) log << "\033[22m";
        log << ": " << msg;
        if (color) log << "\033[0m";
        log << '\n';
      }
      PrintStacktrace(log, boost::stacktrace::stacktrace{}, color);
      log.flush();

      std::abort();
    }
  }
}
