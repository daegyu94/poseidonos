#pragma once

#include <time.h>
#include <stdint.h>

enum {
        MAX_READCACHE_BR,
};

struct readcache_br {
        uint64_t elapseds[MAX_READCACHE_BR];
};

extern struct readcache_br readcache_br;
extern const char *readcache_br_names[];

#ifdef READCACHE_BREAKDOWN

#define _(x)                    readcache_time_##x
#define readcache_declare_ts(x)       struct timespec _(x) = {0, 0}
#define readcache_start_ts(x)         clock_gettime(CLOCK_MONOTONIC, &_(x))
#define readcache_end_ts(name, x)     do {                              \
        struct timespec end = {0, 0};                                   \
		clock_gettime(CLOCK_MONOTONIC, &end);                           \
        readcache_br.elapseds[name] +=									\
                        (end.tv_sec - _(x).tv_sec) * (size_t) 1e9 +     \
                        (end.tv_nsec - _(x).tv_nsec);                   \
} while (0)

#else
#define readcache_declare_ts(x)              do {} while (0)
#define readcache_start_ts(x)                do {} while (0)
#define readcache_end_ts(name, x)            do {} while (0)

#endif
