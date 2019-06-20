#pragma once

#include_next <ctype.h>

#define isascii(c) (((unsigned) (c)) <= 127)
