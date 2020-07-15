#ifndef UUID_AA3FD944_B99F_4F56_B342_FFE9A6ACD9FE
#define UUID_AA3FD944_B99F_4F56_B342_FFE9A6ACD9FE
#pragma once

#include "libshit/lua/type_traits.hpp"
#include "libshit/platform.hpp"

#include <iosfwd>
#include <mutex>

// This two can be overridden in a per cpp basis, for example to have DBG log in
// an otherwise release build, just make sure you define them before
// (indirectly) including logger.hpp.
#ifndef LIBSHIT_IS_DBG_LOG_ENABLED
#  define LIBSHIT_IS_DBG_LOG_ENABLED LIBSHIT_IS_DEBUG
#endif
#ifndef LIBSHIT_LOG_WITH_SOURCE_LOCATION
#  define LIBSHIT_LOG_WITH_SOURCE_LOCATION LIBSHIT_IS_DEBUG
#endif

#if LIBSHIT_LOG_WITH_SOURCE_LOCATION
#  include "libshit/file.hpp" /* IWYU pragma: export */ // IWYU pragma: keep
#endif

namespace Libshit { class OptionGroup; }

namespace Libshit::Logger
{

#undef ERROR // fuck you windows.h

  enum Level
  {
    NONE = -4,
    ERROR = -3,
    WARNING = -2,
    INFO = -1,
  };

  OptionGroup& GetOptionGroup();
  extern int global_level;
  extern bool show_fun;

  bool HasAnsiColor() noexcept;
  bool HasWinColor() noexcept;

  int GetLogLevel(const char* name) noexcept;
  inline bool CheckLog(const char* name, int level) noexcept
  { return GetLogLevel(name) >= level; }
  std::ostream& Log(
    const char* name, int level, const char* file, unsigned line,
    const char* fun);

#if LIBSHIT_LOG_WITH_SOURCE_LOCATION
#  define LIBSHIT_LOG_ARGS LIBSHIT_FILE, __LINE__, LIBSHIT_FUNCTION
#else
#  define LIBSHIT_LOG_ARGS nullptr, 0, nullptr
#endif

  // you can lock manually it if you want to make sure consecutive lines end up
  // in one block
  std::recursive_mutex& GetLogMutex() noexcept;

#define LIBSHIT_LOG(name, level)                     \
  ::Libshit::Logger::GetLogLevel(name) >= (level) && \
  ::Libshit::Logger::Log(name, level, LIBSHIT_LOG_ARGS)

#define LIBSHIT_CHECK_LOG(name, level) \
  (::Libshit::Logger::GetLogLevel(name) >= (level))

#define LIBSHIT_ERR(name)        LIBSHIT_LOG(name, ::Libshit::Logger::ERROR)
#define LIBSHIT_CHECK_ERR(name)  LIBSHIT_CHECK_LOG(name, ::Libshit::Logger::ERROR)
#define LIBSHIT_WARN(name)       LIBSHIT_LOG(name, ::Libshit::Logger::WARNING)
#define LIBSHIT_CHECK_WARN(name) LIBSHIT_CHECK_LOG(name, ::Libshit::Logger::WARNING)
#define LIBSHIT_INF(name)        LIBSHIT_LOG(name, ::Libshit::Logger::INFO)
#define LIBSHIT_CHECK_INF(name)  LIBSHIT_CHECK_LOG(name, ::Libshit::Logger::INFO)

  // silence warnings about null pointer dereference with clang in template
  // instatiations
  extern std::ostream* nullptr_ostream;

#if LIBSHIT_IS_DBG_LOG_ENABLED
#  define LIBSHIT_DBG(name, level)                                              \
  ([]{static_assert(0 <= (level) && (level) < 5, "invalid debug level");},1) && \
  LIBSHIT_LOG(name, level)
#  define LIBSHIT_CHECK_DBG(name, level) LIBSHIT_CHECK_LOG(name, level)
#else
#  define LIBSHIT_DBG(name, level) \
  while (false) *::Libshit::Logger::nullptr_ostream
#  define LIBSHIT_CHECK_DBG(name, level) false
#endif

  // caching for performance: use it if you have a lot of debug logs in a tight
  // loop. Usage: call CACHE_LOGLEVEL() somewhere at the beginning of the func,
  // then use CDBG, CINF, etc, instead of DBG, INF, ... in that function
#define LIBSHIT_CACHE_LOGLEVEL(name) \
  int libshit_log_level_cache = ::Libshit::Logger::GetLogLevel(name)
#if LIBSHIT_IS_DBG_LOG_ENABLED
#  define LIBSHIT_CACHE_DBGLOGLEVEL(name) LIBSHIT_CACHE_LOGLEVEL(name)
#else
#  define LIBSHIT_CACHE_DBGLOGLEVEL(name)
#endif

#define LIBSHIT_CLOG(name, level)       \
  libshit_log_level_cache >= (level) && \
  ::Libshit::Logger::Log(name, level, LIBSHIT_LOG_ARGS)
#define LIBSHIT_CCHECK_LOG(name, level) (libshit_log_level_cache >= (level))

#define LIBSHIT_CERR(name)        LIBSHIT_CLOG(name, ::Libshit::Logger::ERROR)
#define LIBSHIT_CCHECK_ERR(name)  LIBSHIT_CCHECK_LOG(name, ::Libshit::Logger::ERROR)
#define LIBSHIT_CWARN(name)       LIBSHIT_CLOG(name, ::Libshit::Logger::WARNING)
#define LIBSHIT_CCHECK_WARN(name) LIBSHIT_CCHECK_LOG(name, ::Libshit::Logger::WARNING)
#define LIBSHIT_CINF(name)        LIBSHIT_CLOG(name, ::Libshit::Logger::INFO)
#define LIBSHIT_CCHECK_INF(name)  LIBSHIT_CCHECK_LOG(name, ::Libshit::Logger::INFO)
#if LIBSHIT_IS_DBG_LOG_ENABLED
#  define LIBSHIT_CDBG(name, level)                                             \
  ([]{static_assert(0 <= (level) && (level) < 5, "invalid debug level");},1) && \
  LIBSHIT_CLOG(name, level)
#  define LIBSHIT_CCHECK_DBG(name, level) LIBSHIT_CCHECK_LOG(name, level)
#else
#  define LIBSHIT_CDBG(name, level) LIBSHIT_DBG(name, level)
#  define LIBSHIT_CCHECK_DBG(name, level) false
#endif

  // ugly implementation details, don't look
  namespace Detail
  {
    struct GlobalInitializer
    { GlobalInitializer(); ~GlobalInitializer() noexcept; };

    struct PerThreadInitializer
    {
      PerThreadInitializer();
      ~PerThreadInitializer() noexcept;
      struct PerThread* pimpl;
    };
    inline GlobalInitializer global_initializer;
    inline thread_local PerThreadInitializer per_thread_initializer;
  }
}

LIBSHIT_ENUM(Libshit::Logger::Level);

#endif
