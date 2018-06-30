#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <libshit/except.hpp>
#include <libshit/logger.hpp>

#include <string>

static std::string info(const Libshit::Exception& e)
{ return Libshit::ExceptionToString(e); }

int main(int argc, const char** argv)
{
  Catch::ExceptionTranslatorRegistrar x{info};
  Libshit::Logger::global_level = Libshit::Logger::ERROR;

  return Catch::Session().run( argc, argv );
}
