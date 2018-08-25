#include "libshit/except.hpp"

#include "libshit/assert.hpp"

#include <boost/core/demangle.hpp>

#include <atomic>
#include <cstdlib>
#include <map>
#include <typeinfo>

#define LIBSHIT_LOG_NAME "except"
#include "libshit/logger_helper.hpp"

#ifdef WINDOWS
extern "C" void _assert(const char* msg, const char* file, unsigned line);
#endif

namespace Libshit
{
  struct ExceptionInfo
  {
    std::atomic<unsigned> refcount = 0;
    const char* file = nullptr;
    unsigned line = 0;
    const char* func = nullptr;
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

  void Exception::PrintDesc(std::ostream& os) const
  {
    if (!info) return;
    if (info->file) os << "Exception at " << info->file << ':' << info->line;
    if (info->func) os << " in " << info->func;
    if (info->file || info->func) os << "\n";

    for (auto& e : info->map)
      os << e.first << ": " << e.second << '\n';
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
    //RETHROW(std::regex_error)
    RETHROW(std::system_error)
    RETHROW(std::runtime_error)

    RETHROW(std::domain_error)
    RETHROW(std::invalid_argument)
    RETHROW(std::length_error)
    RETHROW(std::out_of_range)
    RETHROW(std::logic_error)
  }

  std::string ExceptionToString(const Exception& e)
  {
    std::stringstream ss;
    auto se = dynamic_cast<const std::exception*>(&e);
    if (se) ss << se->what() << "\n";
    ss << "Type: " << boost::core::demangle(typeid(e).name()) << "\n";
    e.PrintDesc(ss);
    return ss.str();
  }

  std::string ExceptionToString()
  {
    try { throw; }
    catch (const Exception& e)
    {
      return ExceptionToString(e);
    }
    catch (const std::exception& e)
    {
      return e.what();
    }
    catch (...)
    {
      return "Unknown exception (run while you can)";
    }
  }

  void AssertFailed(
    const char* expr, const char* msg, const char* file, unsigned line,
    const char* fun)
  {
#ifdef WINDOWS
    std::string fake_expr = expr;
    if (fun)
    {
      fake_expr += "\nFunction: ";
      fake_expr += fun;
    }
    if (msg)
    {
      fake_expr += "\nMessage: ";
      fake_expr += msg;
    }
    _assert(fake_expr.c_str(), file ? file : "", line);
#else
    auto& log = Logger::Log("assert", Logger::ERROR, file, line, fun);
    log << "Assertion failed!\n";
#  ifdef NDEBUG
    log << file << ':' << line << ": in function " << fun << "\n";
#  else
    if (!Logger::show_fun && fun) log << "in function " << fun << "\n";
#  endif
    log << "Expression: " << expr << '\n';
    if (msg)
      log << "Message: " << msg << '\n';
    log << std::flush;
    std::abort();
#endif
  }
}
