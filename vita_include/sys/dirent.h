#pragma once

#define closedir closedir_temp
#include_next <sys/dirent.h>
#undef closedir

static inline int closedir(DIR* dir) { return 0; }
