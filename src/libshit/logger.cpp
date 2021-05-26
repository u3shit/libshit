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
#include "libshit/nonowning_string.hpp"
#include "libshit/options.hpp"
#include "libshit/string_utils.hpp"
#include "libshit/utils.hpp"

#if LIBSHIT_WITH_LUA
#  include "libshit/logger.lua.h"
#endif

#include <boost/tokenizer.hpp>
#include <Tracy.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

#if !LIBSHIT_OS_IS_WINDOWS
#  include <ctime>
#  include <sys/time.h>
#  include <unistd.h>
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
  std::ostream* nullptr_ostream;

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

        auto x = std::getenv("TERM");
        _256_colors = x && std::strstr(x, "256color");

#if LIBSHIT_OS_IS_WINDOWS
        win_colors = _isatty(2);
#elif !LIBSHIT_OS_IS_VITA
        ansi_colors = isatty(2) && x ? std::strcmp(x, "dummy") != 0 : false;
#endif
      }

      bool win_colors = false;
      bool ansi_colors = false;
      bool _256_colors = true;
      bool print_time = false;

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
  static Global& GetGlobal() noexcept
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
    "Show function signatures in log when available",
    [](auto&, auto&&) { show_fun = true; }};

  static int ParseLevel(StringView str)
  {
    if (!Ascii::CaseCmp(str, "none"_ns))
      return NONE;
    if (!Ascii::CaseCmp(str, "err"_ns) || !Ascii::CaseCmp(str, "error"_ns))
      return ERROR;
    else if (!Ascii::CaseCmp(str, "warn"_ns) || !Ascii::CaseCmp(str, "warning"_ns))
      return WARNING;
    else if (!Ascii::CaseCmp(str, "info"_ns))
      return INFO;
    else
    {
      int l;
      auto res = std::from_chars(str.begin(), str.end(), l);
      if (res.ec != std::errc() || res.ptr != str.end())
        throw InvalidParam{Cat({"Invalid log level ", str})};
      return l;
    }
  }

  static Option debug_level_opt{
    GetOptionGroup(), "log-level", 'l', 1,
    "[MODULE=LEVEL,[...]][DEFAULT_LEVEL]",
    "Sets logging level for the specified modules, or the global default\n\t"
    "Valid levels: none, err, warn, info, 0..4 (debug levels"
#if !LIBSHIT_IS_DBG_LOG_ENABLED
    " - non-debug build, most of them will be missing"
#endif
    ")\n\tDefault level: info",
    [](auto& parser, auto&& args)
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
          global_level = ParseLevel(tok);
        else
        {
          auto lvl = ParseLevel(StringView{tok}.substr(p + 1));
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

  static void EnableAnsiColors(
    OptionParser& parser, std::vector<const char*>&&) noexcept
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
    "Force output colorization with ANSI escape sequences",
    FUNC<EnableAnsiColors>};
  static Option no_colors_opt{
    GetOptionGroup(), "no-colors", 0, nullptr,
    "Disable output colorization",
    [](auto&, auto&&)
    {
      GetGlobal().win_colors = false;
      GetGlobal().ansi_colors = false;
    }};
  static Option print_time_opt{
    GetOptionGroup(), "print-time", 0, nullptr,
    "Print timestamps before log messages",
    [](auto&, auto&&) { GetGlobal().print_time = true; }};

  static constexpr std::uint8_t RAND_COLORS[] = {
    1,2,3,4,5,6,7, 8,9,10,11,12,13,14,15,

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
  static constexpr const std::size_t NON256COL_NUM = 15;

#if LIBSHIT_OS_IS_WINDOWS
  static std::uint8_t WIN_COLOR_MAP[] = {
#define C(r,g,b,i)                                                     \
    (((r) ? FOREGROUND_RED : 0) | ((g) ? FOREGROUND_GREEN : 0) |       \
     ((b) ? FOREGROUND_BLUE : 0) | ((i) ? FOREGROUND_INTENSITY : 0))
    C(0,0,0,0), C(1,0,0,0), C(0,1,0,0), C(1,1,0,0), C(0,0,1,0), C(1,0,1,0),
    C(0,1,1,0), C(1,1,1,0),

    C(0,0,0,1), C(1,0,0,1), C(0,1,0,1), C(1,1,0,1), C(0,0,1,1), C(1,0,1,1),
    C(0,1,1,1), C(1,1,1,1),
#undef C
  };
#endif

  // 32-bit FNV-1a hash http://www.isthe.com/chongo/tech/comp/fnv/index.html
  static std::uint32_t Hash(StringView sv)
  {
    std::uint32_t res = 0x811c9dc5;
    for (unsigned char c : sv)
      res = (res ^ c) * 0x01000193;
    return res;
  }

  static size_t max_name = 16;
  static size_t max_file = 42, max_fun = 20;

  static void IntToStrPadded(std::string& out, unsigned i, std::size_t to_pad,
                             char pad_char = '0')
  {
    char buf[128];
    auto res = std::to_chars(std::begin(buf), std::end(buf), i);
    auto len = res.ptr - buf;
    if (to_pad > len) out.append(to_pad-len, pad_char);
    out.append(buf, len);
  }

  static void PrintTime(std::string& out)
  {
#define F(i, n) IntToStrPadded(out, i, n)

#if LIBSHIT_OS_IS_WINDOWS
    SYSTEMTIME tim;
    GetLocalTime(&tim);
    F(tim.wYear, 0); out += '-'; F(tim.wMonth, 2); out += '-'; F(tim.wDay, 2);
    out += ' '; F(tim.wHour, 2); out += ':'; F(tim.wMinute, 2); out += ':';
    F(tim.wSecond, 2); out += '.'; F(tim.wMilliseconds, 3);
#else
    struct timeval tv;
    if (gettimeofday(&tv, nullptr) < 0) return;
    char buf[128];
    if (strftime(buf, 128, "%F %H:%M:%S.", localtime(&tv.tv_sec)) == 0) return;
    out.append(buf); F(tv.tv_usec, 6);
#endif

    out += ' ';
#undef F
  }

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
          if (buf.empty()) WriteBegin();
          auto end = std::find(msg, msg+n, '\n');
          if (end == msg+n)
          {
            if (lock.owns_lock()) lock.unlock();
            buf.append(msg, msg+n);
            return old_n;
          }

          buf.append(msg, end-msg);
          WriteEnd();
          TracyMessageC(buf.data(), buf.size(), GetTracyColor());

          if (!lock.owns_lock()) lock.lock();
          if (HasAnsiColor())
            os.write(buf.data(), buf.size());
#if LIBSHIT_OS_IS_WINDOWS
          else if (HasWinColor())
            WinFormat();
#endif
          else
            StripFormat();
          buf.clear();

          ++end; // skip \n -- WriteEnd wrote it
          n -= end-msg;
          msg = end;
        }
        return old_n;
      }

      template <typename Cb>
      void ProcessAnsi(Cb cb)
      {
        enum class State { INIT, ESC, CSI } state = State::INIT;
        char* csi_start;
        for (char& c : buf)
          switch (state)
          {
          case State::INIT:
            if (c == 033) state = State::ESC; else os.put(c); break;
          case State::ESC:
            if (c == '[')
            {
              state = State::CSI;
              csi_start = &c+1;
            }
            else
              state = State::INIT;
            break;
          case State::CSI:
            if (c >= 0x40 && c <= 0x7e)
            {
              cb({csi_start, &c}, c);
              state = State::INIT;
            }
            break;
          }
      }

      void StripFormat() { ProcessAnsi([](StringView, char){}); }
#if LIBSHIT_OS_IS_WINDOWS
      void WinFormat()
      {
        constexpr const auto reset =
          FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        constexpr const auto color_mask = reset;
        auto win_attrib = reset;
        auto fun = [&](StringView csi, char cmd)
        {
          if (cmd != 'm') return;
          os.flush();
          auto h = GetStdHandle(STD_ERROR_HANDLE);

          if (csi.empty())
          {
            SetConsoleTextAttribute(h, reset);
            return;
          }

          enum class State { NORMAL, FG0, FG1 } state = State::NORMAL;
          std::size_t p = 0;
          for (auto i = csi.find_first_of(';'); p != StringView::npos;
               i == StringView::npos ? p = i :
                 (p = i+1, i = csi.find_first_of(';', p)))
          {
            unsigned sgr;
            auto sub = csi.substr(p, i-p);
            auto res = std::from_chars(sub.begin(), sub.end(), sgr);
            if (res.ec != std::errc() || res.ptr != sub.end()) continue;
            switch (state)
            {
            case State::NORMAL:
              switch (sgr)
              {
              case 0:  win_attrib = reset; break;
              case 1:  win_attrib |= FOREGROUND_INTENSITY; break;
              case 22: win_attrib &= ~FOREGROUND_INTENSITY; break;
              case 38: state = State::FG0; break;
              }

              if (sgr >= 30 && sgr <= 37)
                win_attrib = (win_attrib & ~color_mask) | WIN_COLOR_MAP[sgr-30];
              break;

            case State::FG0:
              state = sgr == 5 ? State::FG1 : State::NORMAL; break;

            case State::FG1:
              if (sgr < std::size(WIN_COLOR_MAP)) win_attrib = WIN_COLOR_MAP[sgr];
              state = State::NORMAL;
              break;
            }
          }
          SetConsoleTextAttribute(h, win_attrib);
        };
        ProcessAnsi(fun);
      }
#endif

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

      std::uint32_t GetTracyColor() const
      {
        switch (level)
        {
        case ERROR:   return 0xff0000;
        case WARNING: return 0xffff00;
        case INFO:    return 0x00ff00;
        default:      return 0;
        }
      }

      void WriteBegin()
      {
        if (GetGlobal().print_time) PrintTime(buf);

        auto print_col = [&]()
        {
          switch (level)
          {
          case ERROR:   buf.append("\033[0;1;31m"); break;
          case WARNING: buf.append("\033[0;1;33m"); break;
          case INFO:    buf.append("\033[0;1;32m"); break;
          default:      buf.append("\033[0;1m");    break;
          }
        };
        print_col();

        switch (level)
        {
        case ERROR:   buf.append("ERROR"); break;
        case WARNING: buf.append("WARN "); break;
        case INFO:    buf.append("info "); break;
        default:
          buf.append("dbg");
          IntToStrPadded(buf, level, 2, ' ');
          break;
        }

        buf += '[';
        max_name = std::max(max_name, name.size());

        {
          auto i = Hash(name);
          if (!HasWinColor() && GetGlobal()._256_colors)
            i %= std::size(RAND_COLORS);
          else
            i %= NON256COL_NUM;
          buf.append("\033[22;38;5;");
          IntToStrPadded(buf, RAND_COLORS[i], 1);
          buf += 'm';
        }

        buf.append(max_name - name.size(), ' ').append(name);
        print_col();
        buf.append("]\033[22m");

        if (!file.empty())
        {
          max_file = std::max(max_file, file.size());
          buf.append(max_file + 1 - file.size(), ' ').append(file);
          buf += ':'; IntToStrPadded(buf, line, 3, ' ');
        }
        if (show_fun && !fun.empty())
        {
          max_fun = std::max(max_fun, fun.size());
          buf.append(max_fun + 1 - fun.size(), ' ').append(fun);
        }
        buf.append(": ");
      }

      void WriteEnd()
      {
        buf.append("\033[0m\n");
      }

      std::string buf;
      StringView name;
      int level;
      StringView file;
      unsigned line;
      StringView fun;
    };
  }

  int GetLogLevel(const char* name) noexcept
  {
    // inline GetGlobal, for debug builds
    if (auto n = reinterpret_cast<Global*>(&global_storage)->level_size)
    {
      auto l = reinterpret_cast<Global*>(&global_storage)->levels;
      while (n--)
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

    lua_Debug dbg;
    if (lua_getstack(vm, 1, &dbg) && lua_getinfo(vm, "Sln", &dbg))
    {
      file = dbg.short_src;
      line = dbg.currentline;
      fun = dbg.name;
    }

    auto& os = Log(name, level, file, line, fun);

    std::size_t len;
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
