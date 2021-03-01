#ifndef UUID_BCBAF2F6_1339_4E4C_843F_373C736EBC69
#define UUID_BCBAF2F6_1339_4E4C_843F_373C736EBC69
#pragma once

#include "libshit/except.hpp"
#include "libshit/function.hpp"
#include "libshit/utils.hpp"

#include <cstddef>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <vector>

namespace Libshit
{

  struct Exit { bool success; };
  LIBSHIT_GEN_EXCEPTION_TYPE(InvalidParam, std::runtime_error);

  struct Option;
  class OptionParser;

  class OptionGroup final
  {
  public:
    OptionGroup(const char* name, const char* help = nullptr)
      : name{name}, help{help} {}
    OptionGroup(
      OptionParser& parser, const char* name, const char* help = nullptr);

    const char* GetName() const noexcept { return name; }
    const char* GetHelp() const noexcept { return help; }
    const std::vector<Option*>& GetOptions() const noexcept { return options; }

    static OptionGroup& GetCommands();
    static OptionGroup& GetTesting();

  private:
    friend struct Option;
    const char* const name;
    const char* const help;
    std::vector<Option*> options;
  };

  struct Option final
  {
    using ArgVector = std::vector<const char*>;
    using Func = Function<void (OptionParser& parser, ArgVector&&) const>;
    Option(OptionGroup& group, const char* name, char short_name,
           std::size_t args_count, const char* args_help, const char* help,
           Func func)
      : name{name}, short_name{short_name}, args_count{args_count},
        args_help{args_help}, help{help}, func{Move(func)}
    { group.options.push_back(this); }

    Option(OptionGroup& group, const char* name, std::size_t args_count,
           const char* args_help, const char* help, Func func)
      : Option{group, name, 0, args_count, args_help, help, Move(func)} {}

    const char* const name;
    const char short_name;
    static constexpr const std::size_t ALL_ARGS =
      std::numeric_limits<std::size_t>::max();
    const std::size_t args_count;

    const char* const args_help;
    const char* const help;

    const Func func;
    bool enabled = true;
  };

  Option::ArgVector ArgsToVector(int argc, const char** argv);
  inline Option::ArgVector ArgsToVector(int argc, char** argv)
  { return ArgsToVector(argc, const_cast<const char**>(argv)); }

  class OptionParser final
  {
  public:
    OptionParser();
    OptionParser(const OptionParser&) = delete;
    void operator=(const OptionParser&) = delete;

    void SetVersion(const char* version_str);
    const char* GetVersion() const noexcept { return version; }

    void SetUsage(const char* usage) { this->usage = usage; }
    const char* GetUsage() const noexcept { return usage; }

    using NonArgFun = Function<void (const char*)>;
    /// Called on non-arguments (doesn't start with `-` or after `--`)
    void SetNonArgHandler(NonArgFun fun) { non_arg_fun = Move(fun); }
    void FailOnNonArg();
    const NonArgFun& GetNonArgHandler() const noexcept { return non_arg_fun; }

    using ValidateFun = Function<bool (int, const char**)>;
    /**
     * Called after parsing all arguments and non-arguments, can be used for
     * arbitrary validation. Return `false` to fail parse and print help.
     */
    void SetValidateFun(ValidateFun fun) { validate_fun = Move(fun); }
    const ValidateFun& GetValidateFun() const noexcept
    { return validate_fun; }

    void SetShowHelpOnNoOptions(bool show = true) noexcept
    { no_opts_help = show; }
    bool GetShowHelpOnNoOptions() const noexcept { return no_opts_help; }

    void SetOstream(std::ostream& os) { this->os = &os; }
    std::ostream& GetOstream() noexcept { return *os; }

    void Run(int& argc, char** argv, bool has_argv0 = true)
    { Run(argc, const_cast<const char**>(argv), has_argv0); }
    void Run(int& argc, const char** argv, bool has_argv0 = true);

    void Run(Option::ArgVector& args, bool has_argv0);

    void ShowHelp() const;

    static inline OptionParser& GetGlobal()
    {
      static OptionParser inst;
      return inst;
    }

    void CommandTriggered();
    const char* GetArgv0() const noexcept { return argv0; }
    void AddOptionGroup(OptionGroup& grp) { groups.push_back(&grp); }

  private:
    void Run_(int& argc, const char** argv, bool has_argv0);

    std::vector<OptionGroup*> groups;

    const char* version = nullptr;
    const char* usage = nullptr;
    OptionGroup help_version;
    Option help_option, version_option;
    NonArgFun non_arg_fun;
    ValidateFun validate_fun;
    bool no_opts_help = false;
    bool was_command = false;

    std::ostream* os;
    const char* argv0 = nullptr;
  };
}
#endif
