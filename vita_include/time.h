#pragma once

#include_next <time.h>

int nanosleep(const struct timespec* req, struct timespec* rem);
