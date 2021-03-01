// Standalone test runner for libshit, only used when building libshit directly

#include <libshit/options.hpp>

#include <vector>

int main(int argc, char** argv)
{
    auto& parser = Libshit::OptionParser::GetGlobal();
    parser.FailOnNonArg();

    // doctest ignores unknown options, --test eats all options, so just fake a
    // --test at the end of the argument list, this way if the user didn't
    // specify --test, the tests will still run, and if the user specified
    // --test, that extra --test will be ignored...
    auto args = Libshit::ArgsToVector(argc, argv);
    args.push_back("--test");

    try { parser.Run(args, true); }
    catch (const Libshit::Exit& e) { return !e.success; }

    return 0;
}
