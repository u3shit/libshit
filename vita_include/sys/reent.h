#pragma once

#include_next <sys/reent.h>

// cause compilation errors when code tries to use __impure_ptr, instead of
// random link-time errors
#undef _REENT
