#include "libshit/logger.hpp"

#if LIBSHIT_OS_IS_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <io.h>
#  undef ERROR
#endif

#include "libshit/function.hpp"
#include "libshit/lua/function_call.hpp"
#include "libshit/options.hpp"
#include "libshit/utils.hpp"

#if LIBSHIT_WITH_LUA
#  include "libshit/logger.lua.h"
#endif

#include <boost/tokenizer.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

#if !LIBSHIT_OS_IS_WINDOWS
#  include <unistd.h>
#endif

#if LIBSHIT_STDLIB_IS_MSVC
#  define strncasecmp _strnicmp
#  define strcasecmp _stricmp
#else
#  include <strings.h>
#endif

namespace Libshit::Logger
{

  OptionGroup& GetOptionGroup()
  {
    static OptionGroup grp{OptionParser::GetGlobal(), "Logging options"};
    return grp;
  }

  int global_level = -1;
  bool show_fun = false;
#if !LIBSHIT_IS_DEBUG
  std::ostream* nullptr_ostream;
#endif

  namespace
  {
    struct Global
    {
      Global()
      {
        // otherwise clog is actually unbuffered...
        static char buf[4096];
        // buf can be nullptr on linux (and posix?), but crashes on windows...
        setvbuf(stderr, buf, _IOFBF, 4096);
        std::ios_base::sync_with_stdio(false);

#if LIBSHIT_OS_IS_WINDOWS
        win_colors = _isatty(2);
#elif !LIBSHIT_OS_IS_VITA
        const char* x;
        ansi_colors = isatty(2) &&
          (x = getenv("TERM")) ? strcmp(x, "dummy") != 0 : false;
#endif
      }

      bool win_colors = false;
      bool ansi_colors = false;

      std::recursive_mutex log_mutex;
      std::vector<std::pair<const char*, int>> level_map;
      // direct access data/size for checker function below
      const std::pair<const char*, int>* levels = nullptr;
      std::size_t level_size = 0;
      std::vector<std::string> strings;
    };
  }

  // BIG FUGLY HACK: under clang windows, this GlobalInitializer is
  // constructed/destructed multiple times (on the same address!)
  static unsigned global_refcount;
  static std::aligned_storage_t<sizeof(Global), alignof(Global)> global_storage;
  Global& GetGlobal() noexcept
  { return *reinterpret_cast<Global*>(&global_storage); }
  namespace Detail
  {
    GlobalInitializer::GlobalInitializer()
    {
      if (global_refcount++ != 0) return;
      new (&global_storage) Global;
    }
    GlobalInitializer::~GlobalInitializer() noexcept
    {
      if (--global_refcount != 0) return;
      GetGlobal().~Global();
    }
  }

  std::recursive_mutex& GetLogMutex() noexcept { return GetGlobal().log_mutex; }

  static Option show_fun_opt{
    GetOptionGroup(), "show-functions", 0, nullptr,
    LIBSHIT_IS_DEBUG ? "Show function signatures in log"
      : "Ignored for compatibility with debug builds",
    [](auto&&) { show_fun = true; }};

  static int ParseLevel(const char* str)
  {
    if (strcasecmp(str, "none") == 0)
      return NONE;
    if (strncasecmp(str, "err", 3) == 0)
      return ERROR;
    else if (strncasecmp(str, "warn", 4) == 0)
      return WARNING;
    else if (strncasecmp(str, "info", 4) == 0)
      return INFO;
    else
    {
      char* end;
      auto l = std::strtol(str, &end, 10);
      if (*end)
      {
        std::stringstream ss;
        ss << "Invalid log level " << str;
        throw InvalidParam{ss.str()};
      }
      return l;
    }
  }

  static Option debug_level_opt{
    GetOptionGroup(), "log-level", 'l', 1,
    "[MODULE=LEVEL,[...]][DEFAULT_LEVEL]",
    "Sets logging level for the specified modules, or the global default\n\t"
    "Valid levels: none, err, warn, info"
#if LIBSHIT_IS_DEBUG
    ", 0..4 (debug levels)"
#endif
    "\n\tDefault level: info",
    [](auto&& args)
    {
      boost::char_separator<char> sep{","};
      auto arg = args.front();
      boost::tokenizer<boost::char_separator<char>, const char*>
        tokens{arg, arg+strlen(arg), sep};
      auto& g = GetGlobal();
      for (const auto& tok : tokens)
      {
        auto p = tok.find_first_of('=');
        if (p == std::string::npos)
          global_level = ParseLevel(tok.c_str());
        else
        {
          auto lvl = ParseLevel(tok.c_str() + p + 1);
          auto name = tok.substr(0, p);
          auto it = std::find_if(
            g.level_map.begin(), g.level_map.end(),
            [&](const auto& i) { return i.first == name; });
          if (it == g.level_map.end())
          {
            auto& str = g.strings.emplace_back(Move(name));
            g.level_map.emplace_back(str.c_str(), lvl);
          }
          else
            it->second = lvl;
        }
      }

      g.levels = g.level_map.data();
      g.level_size = g.level_map.size();
    }};

  static auto& os = std::clog;

  bool HasAnsiColor() noexcept { return GetGlobal().ansi_colors; }
  bool HasWinColor() noexcept { return GetGlobal().win_colors; }

  static void EnableAnsiColors(std::vector<const char*>&&) noexcept
  {
#if LIBSHIT_OS_IS_WINDOWS
#  ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#    define ENABLE_VIRTUAL_TERMINAL_PROCESSING 4
#  endif
    // This should enable ANSI sequence processing on win10+. Completely
    // untested as I don't use that spyware.
    auto h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE)
    {
      DWORD mode = 0;
      if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
    GetGlobal().win_colors = false;
    GetGlobal().ansi_colors = true;
  }

  static Option ansi_colors_opt{
    GetOptionGroup(), "ansi-colors", 0, nullptr,
    "Force output colorization with ASCII escape sequences",
    FUNC<EnableAnsiColors>};
  static Option no_colors_opt{
    GetOptionGroup(), "no-colors", 0, nullptr,
    "Disable output colorization",
    [](auto&&)
    {
      GetGlobal().win_colors = false;
      GetGlobal().ansi_colors = false;
    }};

  static std::uint8_t rand_colors[] = {
    4,5,6, 12,13,14,

    // (16..231).select{|i| i-=16; b=i%6; i/=6; g=i%6; i/=6; i+g+b>6}
    33, 38, 39, 43, 44, 45, 48, 49, 50, 51, 63, 68, 69, 73, 74, 75, 78, 79, 80,
    81, 83, 84, 85, 86, 87, 93, 98, 99, 103, 104, 105, 108, 109, 110, 111, 113,
    114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 128, 129, 133, 134, 135,
    138, 139, 140, 141, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
    154, 155, 156, 157, 158, 159, 163, 164, 165, 168, 169, 170, 171, 173, 174,
    175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
    190, 191, 192, 193, 194, 195, 198, 199, 200, 201, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
    223, 224, 225, 226, 227, 228, 229, 230, 231
  };

  static size_t max_name = 16;
  static size_t max_file = 42, max_fun = 20;

  namespace
  {
    struct LogBuffer final : public std::streambuf
    {
      std::streamsize xsputn(const char* msg, std::streamsize n) override
      {
        std::unique_lock lock{GetGlobal().log_mutex, std::defer_lock};
        auto old_n = n;
        while (n)
        {
          auto end = std::find(msg, msg+n, '\n');
          if (end == msg+n)
          {
            if (lock.owns_lock()) lock.unlock();
            buf.append(msg, msg+n);
            return old_n;
          }

          if (!lock.owns_lock()) lock.lock();

          WriteBegin();
          if (!buf.empty())
          {
            os.write(buf.data(), buf.size());
            buf.clear();
          }
          os.write(msg, end-msg);
          WriteEnd();

          ++end; // skip \n -- WriteEnd wrote it
          n -= end-msg;
          msg = end;
        }
        return old_n;
      }

      int_type overflow(int_type ch) override
      {
        if (ch != traits_type::eof())
        {
          char c = ch;
          LogBuffer::xsputn(&c, 1);
        }
        return 0;
      }

      int sync() override
      {
        os.flush();
        return 0;
      }


      void WriteBegin()
      {
#if LIBSHIT_OS_IS_WINDOWS
        HANDLE h;
        int color;

        auto win_col = HasWinColor();
        if (win_col)
        {

          os.flush();
          h = GetStdHandle(STD_ERROR_HANDLE);
          switch (level)
          {
          case ERROR:   color = FOREGROUND_RED;                    break;
          case WARNING: color = FOREGROUND_RED | FOREGROUND_GREEN; break;
          case INFO:    color = FOREGROUND_GREEN;                  break;
          default:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
          }
          SetConsoleTextAttribute(h, FOREGROUND_INTENSITY | color);
        }
#endif
        auto ansi_col = HasAnsiColor();
        auto print_col = [&]()
        {
          if (ansi_col)
          {
            switch (level)
            {
            case ERROR:   os << "\033[0;1;31m"; break;
            case WARNING: os << "\033[0;1;33m"; break;
            case INFO:    os << "\033[0;1;32m"; break;
            default:      os << "\033[0;1m";    break;
            }
          }
        };
        print_col();

        switch (level)
        {
        case ERROR:   os << "ERROR"; break;
        case WARNING: os << "WARN "; break;
        case INFO:    os << "info "; break;
        default:      os << "dbg" << level << ' '; break;
        }

        max_name = std::max(max_name, std::strlen(name));
        os << '[';
        if (ansi_col)
        {
          auto i = std::hash<std::string>{}(name) % std::size(rand_colors);
          os << "\033[22;38;5;" << unsigned(rand_colors[i]) << 'm';
        }
        os << std::setw(max_name) << name;
        print_col();
        os << ']';

#if LIBSHIT_OS_IS_WINDOWS
        if (win_col)
        {
          os.flush();
          SetConsoleTextAttribute(h, color);
        }
#endif
        if (ansi_col) os << "\033[22m";

        if (file)
        {
          max_file = std::max(max_file, std::strlen(file));
          os << ' ' << std::setw(max_file) << file << ':'
             << std::setw(3) << line;
        }
        if (show_fun && fun)
        {
          max_fun = std::max(max_fun, std::strlen(fun));
          os << ' ' << std::setw(max_fun) << fun;
        }
        os << ": ";
      }

      void WriteEnd()
      {
#if LIBSHIT_OS_IS_WINDOWS
        if (HasWinColor())
        {
          os.flush();
          SetConsoleTextAttribute(
            GetStdHandle(STD_ERROR_HANDLE),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
#endif
        if (HasAnsiColor()) os << "\033[0m";

        os << '\n';
      }

      std::string buf;
      const char* name;
      int level;
      const char* file;
      unsigned line;
      const char* fun;
    };
  }

  int GetLogLevel(const char* name) noexcept
  {
    // inline GetGlobal, for debug builds
    if (auto n = reinterpret_cast<Global*>(&global_storage)->level_size)
    {
      auto l = reinterpret_cast<Global*>(&global_storage)->levels;
      while (--n)
      {
        if (strcmp(l->first, name) == 0) return l->second;
        ++l;
      }
    }
    return global_level;
  }

  namespace Detail
  {
    struct PerThread
    {
      LogBuffer filter;
      std::ostream log_os{&filter};
    };

    PerThreadInitializer::PerThreadInitializer() { pimpl = new PerThread; }
    PerThreadInitializer::~PerThreadInitializer() noexcept
    { delete pimpl; pimpl = nullptr; }
  }

  std::ostream& Log(
    const char* name, int level, const char* file, unsigned line,
    const char* fun)
  {
    auto p = Detail::per_thread_initializer.pimpl;
    // I don't know whether this is a bug or not, but apparently the tread local
    // can be destroyed while later dtors might still log. Hopefully this will
    // only happen on the main thread in rare cases, so the leak is not that
    // serious.
    if (p == nullptr)
      p = Detail::per_thread_initializer.pimpl = new Detail::PerThread;

    p->filter.name = name;
    p->filter.level = level;
    p->filter.file = file;
    p->filter.line = line;
    p->filter.fun = fun;
    return p->log_os;
  }

#if LIBSHIT_WITH_LUA
  static void LuaLog(
    Lua::StateRef vm, const char* name, int level, Lua::Skip msg)
  {
    (void) msg;
    const char* file = nullptr;
    unsigned line = 0;
    const char* fun = nullptr;

    if constexpr (LIBSHIT_IS_DEBUG)
    {
      lua_Debug dbg;
      if (lua_getstack(vm, 1, &dbg) && lua_getinfo(vm, "Sln", &dbg))
      {
        file = dbg.short_src;
        line = dbg.currentline;
        fun = dbg.name;
      }
    }
    auto& os = Log(name, level, file, line, fun);

    size_t len;
    auto str = luaL_tolstring(vm, 3, &len); // +1
    os.write(str, len);
    lua_pop(vm, 1); // 0

    os << std::flush;
  }

  static Lua::State::Register reg{[](Lua::StateRef vm)
    {
      lua_createtable(vm, 0, 2); // +1

      vm.PushFunction<&CheckLog>(); // +2
      lua_setfield(vm, -2, "check_log"); // +1

      vm.PushFunction<&LuaLog>(); // +2
      lua_setfield(vm, -2, "raw_log"); // +1

      vm.Push(Level::ERROR); // +2
      lua_setfield(vm, -2, "ERROR"); // +1

      vm.Push(Level::WARNING); // +2
      lua_setfield(vm, -2, "WARNING"); // +1

      vm.Push(Level::INFO); // +2
      lua_setfield(vm, -2, "INFO"); // +1

      lua_pushglobaltable(vm); // +2
      vm.SetRecTable("libshit.log", -2); // +1
      lua_pop(vm, 1); // 0

      LIBSHIT_LUA_RUNBC(vm, logger, 0);
    }};

#endif
}
