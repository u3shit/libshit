// Do not include this header in public headers!
// #define LIBSHIT_LOG_NAME "module_name"
// before including this file

#ifndef UUID_C94AAE56_E97F_45DC_A3AB_5475B7C3BDCD
#define UUID_C94AAE56_E97F_45DC_A3AB_5475B7C3BDCD
#pragma once

#include "logger.hpp"
#include <iostream>

#define LOG(level) LIBSHIT_LOG(LIBSHIT_LOG_NAME, level)
#define CHECK_LOG(level) LIBSHIT_CHECK_LOG(LIBSHIT_LOG_NAME, level)

#define ERR        LIBSHIT_ERR(LIBSHIT_LOG_NAME)
#define CHECK_ERR  LIBSHIT_CHECK_ERR(LIBSHIT_LOG_NAME)
#define WARN       LIBSHIT_WARN(LIBSHIT_LOG_NAME)
#define CHECK_WARN LIBSHIT_CHECK_WARN(LIBSHIT_LOG_NAME)
#define INF        LIBSHIT_INF(LIBSHIT_LOG_NAME)
#define CHECK_INF  LIBSHIT_CHECK_INF(LIBSHIT_LOG_NAME)
#define DBG(level) LIBSHIT_DBG(LIBSHIT_LOG_NAME, level)
#define CHECK_DBG(level) LIBSHIT_CHECK_DBG(LIBSHIT_LOG_NAME, level)

#endif
