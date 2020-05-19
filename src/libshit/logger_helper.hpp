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


#define CACHE_LOGLEVEL() LIBSHIT_CACHE_LOGLEVEL(LIBSHIT_LOG_NAME)
#undef CLOG
#define CLOG(level) LIBSHIT_CLOG(LIBSHIT_LOG_NAME, level)
#undef CCHECK_LOG
#define CCHECK_LOG(level) LIBSHIT_CCHECK_LOG(LIBSHIT_LOG_NAME, level)

#undef CERR
#define CERR        LIBSHIT_CERR(LIBSHIT_LOG_NAME)
#undef CCHECK_ERR
#define CCHECK_ERR  LIBSHIT_CCHECK_ERR(LIBSHIT_LOG_NAME)
#undef CWARN
#define CWARN       LIBSHIT_CWARN(LIBSHIT_LOG_NAME)
#undef CCHECK_WARN
#define CCHECK_WARN LIBSHIT_CCHECK_WARN(LIBSHIT_LOG_NAME)
#undef CINF
#define CINF        LIBSHIT_CINF(LIBSHIT_LOG_NAME)
#undef CCHECK_INF
#define CCHECK_INF  LIBSHIT_CCHECK_INF(LIBSHIT_LOG_NAME)
#undef CDBG
#define CDBG(level) LIBSHIT_CDBG(LIBSHIT_LOG_NAME, level)
#undef CCHECK_DBG
#define CCHECK_DBG(level) LIBSHIT_CCHECK_DBG(LIBSHIT_LOG_NAME, level)
