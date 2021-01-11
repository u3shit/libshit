#pragma once

#define _HAS_CHAR16_T_LANGUAGE_SUPPORT 1
// this is a fucking keyword in the fucking language, but clang tries to
// workaround it for old msvc versions, breaking when you create a char16/32
// literal...
using char16_t = decltype(u'a');
using char32_t = decltype(U'a');
#include_next <yvals.h>
