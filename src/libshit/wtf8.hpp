#ifndef GUARD_EXASPERATINGLY_IMAGABLE_GUNLINE_RATFUCKS_3473
#define GUARD_EXASPERATINGLY_IMAGABLE_GUNLINE_RATFUCKS_3473
#pragma once

#include "libshit/nonowning_string.hpp"
#include "libshit/except.hpp"

#include <stdexcept>
#include <string>

namespace Libshit
{

  LIBSHIT_GEN_EXCEPTION_TYPE(Wtf8DecodeError, std::runtime_error);

  std::u16string Wtf8ToWtf16(Libshit::StringView in);
  std::u16string Wtf8ToWtf16LE(Libshit::StringView in);

  std::string Wtf16ToWtf8(Libshit::U16StringView in);
  std::string Wtf16LEToWtf8(Libshit::U16StringView in);

}

#endif
