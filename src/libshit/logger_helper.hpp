// Do not include this header in public headers!
// #define LIBSHIT_LOG_NAME "module_name"
// before including this file

#include "libshit/logger.hpp" // IWYU pragma: export
#include <ostream> // IWYU pragma: export

// redefine everything in case something overrides it... like WARN and doctest
#undef LOG
#define LOG(level) LIBSHIT_LOG(LIBSHIT_LOG_NAME, level)
#undef CHECK_LOG
#define CHECK_LOG(level) LIBSHIT_CHECK_LOG(LIBSHIT_LOG_NAME, level)

#undef ERR
#define ERR        LIBSHIT_ERR(LIBSHIT_LOG_NAME)
#undef CHECK_ERR
#define CHECK_ERR  LIBSHIT_CHECK_ERR(LIBSHIT_LOG_NAME)
#undef WARN
#define WARN       LIBSHIT_WARN(LIBSHIT_LOG_NAME)
#undef CHECK_WARN
#define CHECK_WARN LIBSHIT_CHECK_WARN(LIBSHIT_LOG_NAME)
#undef INF
#define INF        LIBSHIT_INF(LIBSHIT_LOG_NAME)
#undef CHECK_INF
#define CHECK_INF  LIBSHIT_CHECK_INF(LIBSHIT_LOG_NAME)
#undef DBG
#define DBG(level) LIBSHIT_DBG(LIBSHIT_LOG_NAME, level)
#undef CHECK_DBG
#define CHECK_DBG(level) LIBSHIT_CHECK_DBG(LIBSHIT_LOG_NAME, level)
