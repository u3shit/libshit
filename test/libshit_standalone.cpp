// Standalone test runner for libshit, only used when building libshit directly

#include <libshit/options.hpp>

#include <iostream>

int main(int argc, char** argv)
{
    auto& parser = Libshit::OptionParser::GetGlobal();
    parser.FailOnNonArg();
    parser.SetShowHelpOnNoOptions();

    try { parser.Run(argc, argv); }
    catch (const Libshit::Exit& e) { return !e.success; }

    std::cerr << "Nothing to do, please specify --test...\n";
}
