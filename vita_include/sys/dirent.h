#pragma once

#include <psp2/io/dirent.h>

#define dirent SceIoDirent
#pragma push_macro("dirent")
#define dirent dirernt_temp _Pragma("pop_macro(\"dirent\")")

#include_next <sys/dirent.h>
