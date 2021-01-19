#include <libshit/except.hpp>
#include <libshit/function.hpp>
#include <libshit/options.hpp>
#include <libshit/platform.hpp>
#include <libshit/utils.hpp>

#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream> // IWYU pragma: keep
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#if LIBSHIT_OS_IS_WINDOWS
#  include <crtdbg.h> // doctest doesn't include this with libc++
#endif
#define DOCTEST_CONFIG_IMPLEMENT
#include <libshit/doctest.hpp>

using namespace std::string_literals;

namespace Libshit
{

  static std::string xml_file;
  static Option xml_file_opt{
    OptionParser::GetTestingOptions(), "xml-output", 1, "FILE",
    "Save xml output to this file\n"
    "\t(run as \"--xml-output <filename> --test --reporters=libshit-junit\")",
    [](auto&& args) { xml_file = Move(args.front()); }};

  namespace
  {
    // jenkins ignores test suites completely.
    struct JUnitReporter final : doctest::IReporter
    {
      std::stringstream ss;
      doctest::ConsoleReporter conrep;

      std::chrono::high_resolution_clock::time_point start, case_start;
      const doctest::TestCaseData* case_data;
      std::string subcase_str, current_suite;
      std::vector<std::string> failures;
      unsigned case_asserts, total_failures, total_errors, total_tests,
        total_skipped;

      struct Case
      {
        std::chrono::high_resolution_clock::duration duration;
        std::string classname, name;
        std::vector<std::string> failures;
        std::string error;
        unsigned case_asserts;
      };
      std::vector<Case> cases;

      static double ToSeconds(std::chrono::high_resolution_clock::duration dur)
      {
        using Sec = std::chrono::duration<double>;
        return std::chrono::duration_cast<Sec>(dur).count();
      }

      static void CommonEscape(std::string& str)
      {
        static std::regex re{"\033\\[[^m]*m"};
        str = std::regex_replace(str, re, "");
        auto p = [](unsigned char c)
        { return c < 32 && c != 9 && c != 10 && c != 13; };
        str.erase(std::remove_if(str.begin(), str.end(), p), str.end());
        boost::replace_all(str, "&", "&amp;");
        boost::replace_all(str, "<", "&lt;");
      }

      static std::string Escape(std::string str)
      {
        CommonEscape(str);
        boost::replace_all(str, "\"", "&quot;");
        return str;
      }

      static std::string EscapeBody(std::string str)
      {
        CommonEscape(str);
        boost::replace_all(str, ">", "&gt;");
        return str;
      }

      void WriteXml(
        std::ostream& os, std::chrono::high_resolution_clock::duration dur) const
      {
        os << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n"
              "<testsuites errors=\"" << total_errors << "\" failures=\""
           << total_failures << "\" tests=\"" << total_tests
           << "\" time=\"" << ToSeconds(dur) << "\">\n"
           << "  <testsuite name=\"dummy\" tests=\"" << total_tests
           << "\" errors=\"" << total_errors << "\" failures=\""
           << total_failures << "\" skipped=\"" << total_skipped << "\" time=\""
           << ToSeconds(dur) << "\">\n";

        for (const auto& c : cases)
        {
          os << "    <testcase classname=\"" << Escape(c.classname)
             << "\" name=\"" << Escape(c.name) << "\" assertions=\""
             << c.case_asserts << "\" time=\"" << ToSeconds(c.duration)
             << "\">\n";

          for (const auto& f : c.failures)
            os << "      <failure message=\"failure\">"
               << EscapeBody(f) << "</failure>\n";
          if (!c.error.empty())
            os << "      <error message=\"error\">"
               << EscapeBody(c.error) << "</error>\n";

          os << "    </testcase>\n";
        }

        os << "  </testsuite>\n</testsuites>" << std::endl;
      }

      JUnitReporter(const doctest::ContextOptions& opts)
        : conrep{opts, ss} {}

      // called when a query should be reported (listing test cases, printing
      // the version, etc.)
      virtual void report_query(const doctest::QueryData& dat) override {}

      // called when the whole test run starts
      void test_run_start() override
      {
        conrep.test_run_start();
        total_failures = total_errors = total_tests = total_skipped = 0;
        current_suite.clear();
        start = std::chrono::high_resolution_clock::now();
      }
      // called when the whole test run ends (caching a pointer to the input
      // doesn't make sense here)
      void test_run_end(const doctest::TestRunStats& stat) override
      {
        conrep.test_run_end(stat);
        auto dur = std::chrono::high_resolution_clock::now() - start;

        if (!xml_file.empty())
        {
          std::ofstream f{xml_file};
          WriteXml(f, dur);
        }
        else
          WriteXml(std::cout, dur);
      }

      void TCStartCommon()
      {
        subcase_str.clear();
        failures.clear();
        case_asserts = 0;
        case_start = std::chrono::high_resolution_clock::now();
      }
      // called when a test case is started (safe to cache a pointer to the
      // input)
      void test_case_start(const doctest::TestCaseData& data) override
      {
        conrep.test_case_start(data);
        case_data = &data;
        TCStartCommon();
      }

      void TCEndCommon(std::chrono::high_resolution_clock::duration dur)
      {
        if (ss.str() == "\033[0m") ss.str("");

        ++total_tests;
        if (!failures.empty()) ++total_failures;
        if (!ss.str().empty()) ++total_errors;

        cases.push_back(Case{
            dur, case_data->m_test_suite, case_data->m_name + subcase_str,
            Move(failures), ss.str(), case_asserts,
          });
      }
      // called when a test case has ended
      void test_case_end(const doctest::CurrentTestCaseStats& stat) override
      {
        auto dur = std::chrono::high_resolution_clock::now() - start;
        ss.str("");
        conrep.test_case_end(stat);
        TCEndCommon(dur);
      }
      // called when a test case is reentered because of unfinished subcases
      // (safe to cache a pointer to the input)
      void test_case_reenter(const doctest::TestCaseData& data) override
      {
        auto dur = std::chrono::high_resolution_clock::now() - start;
        ss.str("");
        conrep.test_case_reenter(data);
        TCEndCommon(dur);
        TCStartCommon();
      }

      // called when an exception is thrown from the test case (or it crashes)
      virtual void test_case_exception(
        const doctest::TestCaseException& exc) override
      {
        ss.str("");
        conrep.test_case_exception(exc);
        failures.push_back(ss.str());
      }

      // called whenever a subcase is entered (don't cache pointers to the input)
      void subcase_start(const doctest::SubcaseSignature& sig) override
      {
        conrep.subcase_start(sig);
        subcase_str += "/"s + sig.m_name.c_str();
      }
      // called whenever a subcase is exited (don't cache pointers to the input)
      void subcase_end() override
      {
        conrep.subcase_end();
      }

      // called for each assert (don't cache pointers to the input)
      void log_assert(const doctest::AssertData& dat) override
      {
        ++case_asserts;

        ss.str("");
        conrep.hasLoggedCurrentTestStart = true;
        conrep.log_assert(dat);
        if (!ss.str().empty()) failures.push_back(ss.str());
      }
      // called for each message (don't cache pointers to the input)
      void log_message(const doctest::MessageData& data) override
      {
        conrep.log_message(data);
      }

      // called when a test case is skipped either because it doesn't pass the
      // filters, has a skip decorator or isn't in the execution range (between
      // first and last) (safe to cache a pointer to the input)
      void test_case_skipped(const doctest::TestCaseData& data) override
      {
        conrep.test_case_skipped(data); // doesn't do anything
      }
    };

    REGISTER_REPORTER("libshit-junit", 10, JUnitReporter);
  }

  REGISTER_EXCEPTION_TRANSLATOR(const Libshit::Exception& e)
  { return Libshit::ExceptionToString(e, false).c_str(); }

  static void Fun(std::vector<const char*>&& args)
  {
    doctest::Context ctx;

    ctx.setOption("exit", true);
    ctx.applyCommandLine(args.size(), args.data());
    auto ret = ctx.run();
    if (ctx.shouldExit()) throw Exit{!ret};
  }

  static Option opt{
    OptionParser::GetTestingOptions(), "test", std::size_t(-1), "ARGS...",
    "Run doctest tests (\"--test --help\" for details)\n\t"
    "Remaining arguments will be passed to doctest", FUNC<Fun>};
}
