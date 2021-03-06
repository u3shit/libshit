#include "libshit/options.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>

#include "libshit/doctest.hpp"

namespace Libshit
{
  TEST_SUITE_BEGIN("Libshit::Options");
  namespace
  {
    struct TestFixture
    {
      std::stringstream ss;
      OptionParser parser;

      TestFixture() { parser.SetOstream(ss); }

      void Run(int& argc, const char** argv, bool val)
      {
        try
        {
          parser.Run(argc, argv);
          FAIL("Expected parser to throw Exit, but didn't throw anything");
        }
        catch (const Exit& exit)
        {
          CHECK(exit.success == val);
        }
      }
    };
  }

#define CHECK_STREQ(a_, b_)                         \
  do                                                \
  {                                                 \
    const char* a = a_, * b = b_;                   \
    if (a == nullptr || b == nullptr)               \
      CHECK(a == b);                                \
    else                                            \
    {                                               \
      CAPTURE(a); CAPTURE(b);                       \
      FAST_CHECK_EQ(strcmp(a, b), 0);               \
    }                                               \
  }                                                 \
  while (0)


  OptionGroup::OptionGroup(
    OptionParser& parser, const char* name, const char* help)
    : name{name}, help{help}
  { parser.AddOptionGroup(*this); }

  OptionGroup& OptionGroup::GetCommands()
  {
    static OptionGroup inst{OptionParser::GetGlobal(), "Commands",
      "You can only specify one of these. "
      "Run \"--<command> --help\" for details\n"};
    return inst;
  }

  OptionGroup& OptionGroup::GetTesting()
  {
    static OptionGroup inst{OptionParser::GetGlobal(), "Testing options"};
    return inst;
  }


  Option::ArgVector ArgsToVector(int argc, const char** argv)
  {
    std::vector<const char*> res;
    res.reserve(argc);
    for (int i = 0; i < argc; ++i) res.push_back(argv[i]);
    return res;
  }

  TEST_CASE("ArgsToVector")
  {
    const char* argv[] = { "foo", "bar", "asd", nullptr };
    auto v = ArgsToVector(3, argv);
    REQUIRE(v.size() == 3);
    CHECK_STREQ(v[0], "foo");
    CHECK_STREQ(v[1], "bar");
    CHECK_STREQ(v[2], "asd");
  }

  OptionParser::OptionParser()
    : help_version{*this, "General options"},
      help_option{
        help_version, "help", 'h', 0, nullptr, "Show this help message",
        [this](auto&, auto&&) { this->ShowHelp(); throw Exit{true}; }},
      version_option{
        help_version, "version", 0, nullptr, "Show program version",
        [this](auto&, auto&&) { *os << version << std::endl; throw Exit{true}; }},
      os{&std::clog}
  {
    version_option.enabled = false;
  }

  void OptionParser::SetVersion(const char* version_str)
  {
    version = version_str;
    version_option.enabled = true;
  }

  TEST_CASE_FIXTURE(TestFixture, "version displaying")
  {
    parser.SetVersion("Foo program bar");

    int argc = 2;
    const char* argv[] = { "foo", "--help", nullptr };

    SUBCASE("help message")
    {
      Run(argc, argv, true);
      CHECK(ss.str() ==
            "Foo program bar\n"
            "\n"
            "General options:\n"
            " -h --help\n"
            "\tShow this help message\n"
            "    --version\n"
            "\tShow program version\n");
    }
    SUBCASE("version display")
    {
      argv[1] = "--ver";
      Run(argc, argv, true);
      CHECK(ss.str() == "Foo program bar\n");
    }
  }

  TEST_CASE_FIXTURE(TestFixture, "usage displaying")
  {
    parser.SetUsage("[--options] [bar...]");

    int argc = 2;
    const char* argv[] = { "foo", "--help", nullptr };

    Run(argc, argv, true);
    CHECK(ss.str() ==
          "Usage:\n"
          "\tfoo [--options] [bar...]\n"
          "\n"
          "General options:\n"
          " -h --help\n"
          "\tShow this help message\n");
  }


  void OptionParser::FailOnNonArg()
  {
    non_arg_fun = [](auto) { throw InvalidParam{"Invalid option"}; };
  }

  namespace
  {
    struct OptCmp
    {
      bool operator()(const char* a, const char* b) const
      {
        while (*a && *a == *b) ++a, ++b;

        if (*a == '=' || *b == '=') return false;
        return *reinterpret_cast<const unsigned char*>(a) <
          *reinterpret_cast<const unsigned char*>(b);
      }
    };
  }

  static size_t ParseShort(
    OptionParser& parser, const std::array<Option*, 256>& short_opts,
    size_t argc, size_t i, const char** argv)
  {
    auto ptr = argv[i];
    return AddInfo([&]() -> size_t
    {
      while (*++ptr)
      {
        auto opt = short_opts[static_cast<unsigned char>(*ptr)];
        if (!opt) throw InvalidParam{"Unknown option"};

        std::vector<const char*> args;
        if (opt->args_count == Option::ALL_ARGS)
        {
          args.reserve(argc-i);
          if (ptr[1]) args.push_back(ptr+1);
          std::copy(argv+i+1, argv+argc, std::back_inserter(args));
          opt->func(parser, Move(args));
          return argc - i - 1;
        }

        if (opt->args_count)
        {
          auto count = opt->args_count;
          args.reserve(count);

          if (ptr[1])
          {
            args.push_back(ptr+1);
            --count;
          }

          ++i;
          for (size_t j = 0; j < count; ++j)
          {
            if (i+j >= argc) throw InvalidParam{"Not enough arguments"};

            args.push_back(argv[i+j]);
          }

          opt->func(parser, Move(args));
          return count;
        }

        opt->func(parser, Move(args));
      }
      return 0;
    }, [&](auto& e) { AddInfos(e, "Processed option", std::string{'-', *ptr}); });
  }

  static size_t ParseLong(
    OptionParser& parser,
    const std::map<const char*, Option*, OptCmp>& long_opts,
    size_t argc, size_t i, const char** argv)
  {
    auto arg = strchr(argv[i]+2, '=');
    auto len = arg ? arg - argv[i] - 2 : strlen(argv[i]+2);

    auto it = long_opts.lower_bound(argv[i]+2);
    if (it == long_opts.end() || strncmp(argv[i]+2, it->first, len))
      throw InvalidParam{"Unknown option"};

    if (std::next(it) != long_opts.end() &&
        it->first[len] != '\0' && // exact matches are non ambiguous
        strncmp(argv[i]+2, std::next(it)->first, len) == 0)
    {
      std::stringstream ss;
      ss << "Ambiguous option (candidates:";
      for (; it != long_opts.end() && strncmp(argv[i]+2, it->first, len) == 0;
           ++it)
        ss << " --" << it->first;
      ss << ")";
      throw InvalidParam{ss.str()};
    }

    auto opt = it->second;

    auto count = opt->args_count;
    std::vector<const char*> args;
    if (count == Option::ALL_ARGS)
    {
      args.reserve(argc-i);
      if (arg) args.push_back(arg+1);
      std::copy(argv+i+1, argv+argc, std::back_inserter(args));
      opt->func(parser, Move(args));
      return argc - i - 1;
    }

    args.reserve(count);
    if (arg)
      if (opt->args_count == 0)
        throw InvalidParam{"Option doesn't take arguments"};
      else
      {
        args.push_back(arg+1);
        --count;
      }

    ++i;
    for (size_t j = 0; j < count; ++j)
    {
      if (i+j >= argc) throw InvalidParam{"Not enough arguments"};
      args.push_back(argv[i+j]);
    }

    opt->func(parser, Move(args));
    return count;
  }

  void OptionParser::Run_(int& argc, const char** argv, bool has_argv0)
  {
    if (has_argv0) argv0 = argv[0];
    int endp = has_argv0;
    if (argc == has_argv0)
    {
      if (no_opts_help || (validate_fun && !validate_fun(argc, argv)))
      {
        ShowHelp();
        throw Exit{false};
      }
      else return;
    }

    std::array<Option*, 256> short_opts;
    static_assert(CHAR_BIT == 8);
    std::map<const char*, Option*, OptCmp> long_opts;

    std::size_t old_len = 0;
    auto gen_opts = [&]()
    {
      if (old_len == groups.size()) return;
      old_len = groups.size();

      short_opts = {};
      long_opts.clear();
      for (auto g : groups)
        for (auto o : g->GetOptions())
        {
          if (!o->enabled) continue;
          if (o->short_name)
          {
            if (short_opts[static_cast<unsigned char>(o->short_name)])
              LIBSHIT_THROW(std::logic_error, "Duplicate short option");
            short_opts[static_cast<unsigned char>(o->short_name)] = o;
          }

          auto x = long_opts.emplace(o->name, o);
          if (!x.second)
            LIBSHIT_THROW(std::logic_error, "Duplicate long option");
        }
    };

    for (int i = has_argv0; i < argc; ++i)
    {
      // option: "--"something, "-"something
      // non option: something, "-"
      // special: "--"

      // non option
      if (argv[i][0] != '-' || (argv[i][0] == '-' && argv[i][1] == '\0'))
      {
        if (non_arg_fun) non_arg_fun(argv[i]);
        else argv[endp++] = argv[i];
      }
      else
      {
        gen_opts();
        if (argv[i][1] != '-') // short
          i += ParseShort(*this, short_opts, argc, i, argv);
        else if (argv[i][1] == '-' && argv[i][2] != '\0') // long
          i += AddInfo(
            [&] { return ParseLong(*this, long_opts, argc, i, argv); },
            [=](auto& e) { AddInfos(e, "Processed option", argv[i]); });
        else // --: end of args
        {
          if (non_arg_fun)
            for (++i; i < argc; ++i)
              non_arg_fun(argv[i]);
          else
            for (++i; i < argc; ++i)
              argv[endp++] = argv[i];
          break;
        }
      }
    }

    argc = endp;
    argv[argc] = nullptr;

    if (validate_fun && !validate_fun(argc, argv))
    {
      ShowHelp();
      throw Exit{false};
    }
  }

  void OptionParser::Run(int& argc, const char** argv, bool has_argv0)
  {
    try { Run_(argc, argv, has_argv0); }
    catch (const InvalidParam& p)
    {
      *os << p["Processed option"] << ": " << p.what() << std::endl;
      throw Exit{false};
    }
  }

  void OptionParser::Run(Option::ArgVector& args, bool has_argv0)
  {
    int size = args.size();
    AtScopeExit x{[&]() { args.resize(size); }};
    args.push_back(nullptr);

    try { Run_(size, args.data(), has_argv0); }
    catch (const InvalidParam& p)
    {
      *os << p["Processed option"] << ": " << p.what() << std::endl;
      throw Exit{false};
    }
  }

  void OptionParser::ShowHelp() const
  {
    if (version) *os << version << "\n\n";
    if (usage) *os << "Usage:\n\t" << argv0 << " " << usage << "\n\n";

    for (auto g : groups)
    {
      if (g != groups[0]) *os << '\n';
      *os << g->GetName() << ":\n";
      if (g->GetHelp()) *os << g->GetHelp() << '\n';

      for (auto o : g->GetOptions())
      {
        if (!o->enabled) continue;
        if (o->short_name) *os << " -" << o->short_name;
        else *os << "   ";

        *os << " --" << o->name;
        if (o->args_help) *os << '=' << o->args_help;
        *os << '\n';
        if (o->help) *os << '\t' << o->help << '\n';
      }
    }
  }

  void OptionParser::CommandTriggered()
  {
    if (was_command)
      throw InvalidParam{"Multiple commands specified"};
    was_command = true;
  }

  TEST_CASE_FIXTURE(TestFixture, "basic option parsing")
  {
    OptionGroup grp{parser, "Foo", "Description of foo"};

    bool b1 = false, b2 = false;
    const char* b3 = nullptr, *b40 = nullptr, *b41 = nullptr;

    Option test1{grp, "test-1", 't', 0, nullptr, "foo",
      [&](auto&, auto&&){ b1 = true; }};
    Option test2{grp, "test-2", 'T', 0, nullptr, "bar",
      [&](auto&, auto&&){ b2 = true; }};
    Option test3{grp, "test-3", 1, "STRING", "Blahblah",
      [&](auto&, auto&& v){ REQUIRE(v.size() == 1); b3 = v[0]; }};
    Option test4{grp, "test-4", 'c', 2, "FOO BAR", nullptr,
      [&](auto&, auto&& v){ REQUIRE(v.size() == 2); b40 = v[0]; b41 = v[1]; }};

    bool e1 = false, e2 = false;
    const char* e3 = nullptr, *e40 = nullptr, *e41 = nullptr;

    const char* help_text =
      "General options:\n"
      " -h --help\n"
      "\tShow this help message\n"
      "\n"
      "Foo:\n"
      "Description of foo\n"
      " -t --test-1\n"
      "\tfoo\n"
      " -T --test-2\n"
      "\tbar\n"
      "    --test-3=STRING\n"
      "\tBlahblah\n"
      " -c --test-4=FOO BAR\n";

    int argc = 2;
    const char* argv[10] = { "foo" };

    SUBCASE("success")
    {
      SUBCASE("single option")
      {
        argv[1] = "--test-2";
        parser.Run(argc, argv);
        e2 = true;
      }

      SUBCASE("multiple options")
      {
        argc = 4;
        argv[1] = "--test-2";
        argv[2] = "--test-1";
        argv[3] = "--test-2";
        parser.Run(argc, argv);

        e1 = e2 = true;
      }

      SUBCASE("argument options")
      {
        SUBCASE("= arguments")
        {
          argc = 4;
          argv[1] = "--test-4=xx";
          argv[2] = "yy";
          argv[3] = "--test-3=foo";
        }
        SUBCASE("separate argument options")
        {
          argc = 6;
          argv[1] = "--test-4";
          argv[2] = "xx";
          argv[3] = "yy";
          argv[4] = "--test-3";
          argv[5] = "foo";
        }
        parser.Run(argc, argv);

        e3 = "foo";
        e40 = "xx";
        e41 = "yy";
      }

      SUBCASE("short argument")
      {
        argv[1] = "-t";
        parser.Run(argc, argv);
        e1 = true;
      }

      SUBCASE("short arguments concat")
      {
        argv[1] = "-tT";
        parser.Run(argc, argv);
        e1 = e2 = true;
      }

      SUBCASE("short arguments parameter")
      {
        SUBCASE("normal")
        {
          argc = 4;
          argv[1] = "-tc";
          argv[2] = "abc";
          argv[3] = "def";
        }
        SUBCASE("concat")
        {
          argc = 3;
          argv[1] = "-tcabc";
          argv[2] = "def";
        }
        parser.Run(argc, argv);
        e1 = true;
        e40 = "abc";
        e41 = "def";
      }

      REQUIRE(argc == 1);
      CHECK_STREQ(argv[0], "foo");
      CHECK_STREQ(argv[1], nullptr);
    }

    SUBCASE("unused args")
    {
      argc = 5;
      argv[1] = "--test-1";
      argv[2] = "foopar";
      argv[3] = "barpar";
      argv[4] = "-T";

      parser.Run(argc, argv);
      e1 = e2 = true;
      REQUIRE(argc == 3);
      CHECK_STREQ(argv[0], "foo");
      CHECK_STREQ(argv[1], "foopar");
      CHECK_STREQ(argv[2], "barpar");
    }

    SUBCASE("unused -- teminating")
    {
      argc = 4;
      argv[1] = "-t";
      argv[2] = "--";
      argv[3] = "-T";
      parser.Run(argc, argv);
      e1 = true;

      REQUIRE(argc == 2);
      CHECK_STREQ(argv[0], "foo");
      CHECK_STREQ(argv[1], "-T");
      CHECK_STREQ(argv[2], nullptr);
    }

    SUBCASE("unused handler")
    {
      argc = 6;
      argv[1] = "-t";
      argv[2] = "foopar";
      argv[3] = "-T";
      argv[4] = "--";
      argv[5] = "--help";

      std::vector<const char*> vec;
      parser.SetNonArgHandler([&](auto x) { vec.push_back(x); });
      parser.Run(argc, argv);
      e1 = e2 = true;

      CHECK(vec == (std::vector<const char*>{"foopar", "--help"}));
      REQUIRE(argc == 1);
      CHECK_STREQ(argv[0], "foo");
      CHECK_STREQ(argv[1], nullptr);
    }

    SUBCASE("validator")
    {
      argc = 6;
      argv[1] = "foo";
      argv[2] = "-t";
      argv[3] = "bar";
      argv[4] = "--";
      argv[5] = "-t";
      e1 = true;

      bool valid;
      SUBCASE("valid") { valid = true; }
      SUBCASE("invalid") { valid = false; }

      parser.SetValidateFun(
        [valid](int argc, const char** argv)
        {
          REQUIRE(argc == 4);
          CHECK_STREQ(argv[0], "foo");
          CHECK_STREQ(argv[1], "foo");
          CHECK_STREQ(argv[2], "bar");
          CHECK_STREQ(argv[3], "-t");
          return valid;
        });
      if (valid)
      {
        parser.Run(argc, argv);
        CHECK(ss.str() == "");
      }
      else
      {
        Run(argc, argv, false);
        CHECK(ss.str() == help_text);
      }
    }

    SUBCASE("empty args")
    {
      argc = 1;
      parser.Run(argc, argv);
      REQUIRE(argc == 1);
      CHECK_STREQ(argv[0], "foo");
      CHECK_STREQ(argv[1], nullptr);
    }

    SUBCASE("empty show help")
    {
      argc = 1;
      parser.SetShowHelpOnNoOptions();
      Run(argc, argv, false);
      CHECK(ss.str() == help_text);
    }

    SUBCASE("show help")
    {
      SUBCASE("normal") { argv[1] = "--help"; }
      SUBCASE("abbrev") { argv[1] = "--he"; }
      Run(argc, argv, true);
      CHECK(ss.str() == help_text);
    }

    SUBCASE("ambiguous option")
    {
      argv[1] = "--test";
      Run(argc, argv, false);
      CHECK(ss.str() ==
            "--test: Ambiguous option (candidates: --test-1 --test-2 --test-3 --test-4)\n");
    }

    SUBCASE("ambiguous option with params")
    {
      argv[1] = "--test=foo";
      Run(argc, argv, false);
      CHECK(ss.str() ==
            "--test=foo: Ambiguous option (candidates: --test-1 --test-2 --test-3 --test-4)\n");
    }

    SUBCASE("unknown option")
    {
      argv[1] = "--foo";
      Run(argc, argv, false);
      CHECK(ss.str() == "--foo: Unknown option\n");
    }

    SUBCASE("unknown short option")
    {
      argv[1] = "-x";
      Run(argc, argv, false);
      CHECK(ss.str() == "-x: Unknown option\n");
    }

    CHECK(b1 == e1);
    CHECK(b2 == e2);
    CHECK_STREQ(b3, e3);
    CHECK_STREQ(b40, e40);
    CHECK_STREQ(b41, e41);
  }

  TEST_CASE_FIXTURE(TestFixture, "non-unique prefix")
  {
    OptionGroup grp{parser, "Foo", "Description of foo"};

    bool b1 = false, b2 = false;

    Option test1{grp, "foo",     0, nullptr, "foo",
      [&](auto&, auto&&){ b1 = true; }};
    Option test2{grp, "foo-bar", 0, nullptr, "bar",
      [&](auto&, auto&&){ b2 = true; }};

    int argc = 2;
    const char* argv[] = { "foo", "--foo", nullptr };

    parser.Run(argc, argv);
    CHECK(ss.str() == "");
    CHECK(b1 == true);
    CHECK(b2 == false);
  }

  TEST_CASE_FIXTURE(TestFixture, "non-unique prefix with options")
  {
    OptionGroup grp{parser, "Foo", "Description of foo"};

    const char* b1 = nullptr;
    bool b2 = false;

    Option test1{grp, "foo",     1, nullptr, "foo",
      [&](auto&, auto&& a){ b1 = a.front(); }};
    Option test2{grp, "foo-bar", 0, nullptr, "bar",
      [&](auto&, auto&&){ b2 = true; }};

    int argc;
    const char* argv[4] = {"foo"};
    SUBCASE("space separated")
    {
      argc = 3;
      argv[1] = "--foo";
      argv[2] = "bar";
    }
    SUBCASE("= separated")
    {
      argc = 2;
      argv[1] = "--foo=bar";
    }

    parser.Run(argc, argv);
    CHECK(ss.str() == "");
    CHECK_STREQ(b1, "bar");
    CHECK(b2 == false);
  }

  TEST_CASE_FIXTURE(TestFixture, "command like usage")
  {
    parser.FailOnNonArg();
    OptionGroup grp{parser, "Commands"};

    bool b1x = false, b1y = false, b2x = false;
    OptionGroup cmd1_grp{"Command 1 shit"};
    Option cmd1_x{cmd1_grp, "foo", 0, nullptr, "foo",
      [&](auto&, auto&&) { b1x = true; }};
    Option cmd1_y{cmd1_grp, "bar", 0, nullptr, "baz",
      [&](auto&, auto&&) { b1y = true; }};

    Option cmd1{grp, "cmd1", 0, nullptr, "cmd1", [&](OptionParser& p, auto&& args)
    {
      CHECK(&p == &parser);
      CHECK(args.size() == 0);
      p.CommandTriggered();
      p.AddOptionGroup(cmd1_grp);
    }};

    OptionGroup cmd2_grp{"Command 2 shit"};
    Option cmd2_x{cmd2_grp, "foo", 0, nullptr, "foo2",
      [&](auto&, auto&&) { b2x = true; }};

    bool was_validator = false;
    Option cmd2{grp, "cmd2", 0, nullptr, "cmd2", [&](OptionParser& p, auto&&)
    {
      p.CommandTriggered();
      p.AddOptionGroup(cmd2_grp);
      p.SetNonArgHandler({});
      p.SetValidateFun([&](int argc, const char** argv)
      {
        was_validator = true;
        CHECK(argc == 2);
        CHECK_STREQ(argv[0], "cmd_name");
        CHECK_STREQ(argv[1], "booboo");
        return true;
      });
    }};

    int argc = 1;
    const char* argv[4] = {"cmd_name"};
    std::string base_help_text =
      "General options:\n"
      " -h --help\n"
      "\tShow this help message\n\n"
      "Commands:\n"
      "    --cmd1\n\tcmd1\n"
      "    --cmd2\n\tcmd2\n";

    SUBCASE("normal help")
    {
      argv[argc++] = "--help";
      argv[argc++] = "--cmd1"; // ignored
      Run(argc, argv, true);
      CHECK(ss.str() == base_help_text);
    }
    SUBCASE("help inside cmd1")
    {
      argv[argc++] = "--cmd1";
      argv[argc++] = "--help";
      Run(argc, argv, true);
      CHECK(ss.str() == base_help_text + "\n"
        "Command 1 shit:\n"
        "    --foo\n\tfoo\n"
        "    --bar\n\tbaz\n");
    }
    SUBCASE("help inside cmd2")
    {
      argv[argc++] = "--cmd2";
      argv[argc++] = "--help";
      Run(argc, argv, true);
      CHECK(ss.str() == base_help_text + "\n"
        "Command 2 shit:\n"
        "    --foo\n\tfoo2\n");
    }

    SUBCASE("foo")
    {
      bool b1x_exp = false, b2x_exp = false;
      SUBCASE("cmd1") { argv[argc++] = "--cmd1"; b1x_exp = true; }
      SUBCASE("cmd2") { argv[argc++] = "--cmd2"; b2x_exp = true; }
      argv[argc++] = "--foo";
      if (b2x_exp) argv[argc++] = "booboo";
      parser.Run(argc, argv);
      CHECK(ss.str() == "");
      CHECK(b1x == b1x_exp);
      CHECK(b1y == false);
      CHECK(b2x == b2x_exp);
    }

    SUBCASE("bar in cmd1")
    {
      argv[argc++] = "--cmd1";
      argv[argc++] = "--bar";
      parser.Run(argc, argv);
      CHECK(ss.str() == "");
      CHECK(b1x == false);
      CHECK(b1y == true);
      CHECK(b2x == false);
    }
    SUBCASE("foo in cmd2")
    {
      argv[argc++] = "--cmd2";
      argv[argc++] = "--bar";
      argv[argc++] = "booboo";
      Run(argc, argv, false);
      CHECK(ss.str() == "--bar: Unknown option\n");
    }

    SUBCASE("dup command invalid")
    {
      SUBCASE("cmd1") argv[argc++] = "--cmd1";
      SUBCASE("cmd2") argv[argc++] = "--cmd2";
      argv[argc++] = "--cmd2";
      Run(argc, argv, false);
      CHECK(ss.str() == "--cmd2: Multiple commands specified\n");
    }
  }

  TEST_SUITE_END();
}
