#pragma once

#include <stdint.h>

//#define LATENCY_BREAKDOWN

namespace pos {
enum readcache_stat_name_t {
    RC_ADMIT_SUCC,
    RC_ADMIT_CONTAINED_SUCC,
    RC_ADMIT_LOCKED_FAILED,
    RC_ADMIT_UNALLOC_FAILED,
 
    RC_HIT,
    RC_MISS,
   
    RC_SINGLE_HIT,
    RC_SINGLE_MISS,

    RC_MERGED_HIT,
    RC_MERGED_MISS,
    RC_MERGED_PARTIAL_MISS,
    
    RC_UPDATE_SUCC,
    RC_EVICT_SUCC,
    RC_EVICT_FAILED,
    
    RC_ISSUED_IO,
    
    MAX_READCACHE_STAT,
};

struct readcache_stat {
    uint64_t cnts[MAX_READCACHE_STAT];
    
    uint64_t pool_used = 0;
    uint64_t cache_evict = 0;

    uint64_t read_cnt = 0;
    uint64_t write_cnt = 0;
    bool write_intensive = false;
};

extern struct readcache_stat readcache_stat;
extern const char *readcache_stat_names[];

static inline void readcache_stat_inc(int stat_name, int cnt = 1)
{
    readcache_stat.cnts[stat_name] += cnt;
}

int readcache_stat_spawn_monitor(void);

enum {
    BR_SEG_CACHE, 
    BR_SEG_ALLOC, 
    BR_RBA_LOCK, 
    BR_ISSUE_IO, 

    BR_MAX,
};

struct LatencyBreakdown {
    uint64_t elapseds[BR_MAX];
};

extern struct LatencyBreakdown br;
extern const char *br_names[];

static inline uint64_t elapsed_us(int name)
{
    return br.elapseds[name] / 1000;
}

static inline double elapsed_avg_us(int name, uint64_t cnt)
{
    if (cnt == 0) {
        return 0.0;
    } else{
        return (double) br.elapseds[name] / 1000 / cnt;
    }
}

#ifdef LATENCY_BREAKDOWN

#define _(x)                    br_time_##x
#define br_declare_ts(x)        struct timespec _(x) = {0, 0}
#define br_start_ts(x)          clock_gettime(CLOCK_MONOTONIC, &_(x))
#define br_end_ts(x, name)      do {                                \
    struct timespec end = {0, 0};                                   \
    clock_gettime(CLOCK_MONOTONIC, &end);                           \
    br.elapseds[name] +=								            \
    (end.tv_sec - _(x).tv_sec) * (size_t) 1e9 +                     \
    (end.tv_nsec - _(x).tv_nsec);                                   \
} while (0)
#define br_end_ts_with_lat(x, name, lat)      do {                  \
    struct timespec end = {0, 0};                                   \
    clock_gettime(CLOCK_MONOTONIC, &end);                           \
    br.elapseds[name] += lat +								        \
    (end.tv_sec - _(x).tv_sec) * (size_t) 1e9 +                     \
    (end.tv_nsec - _(x).tv_nsec);                                   \
} while (0)

#define br_add_lat(name, lat)      do {                             \
    br.elapseds[name] += lat;								        \
} while (0)

#else

#define br_declare_ts(x)              do {} while (0)
#define br_start_ts(x)                do {} while (0)
#define br_end_ts(name, x)            do {} while (0)
#define br_end_ts_with_lat(name, x)   do {} while (0)
#define br_add_lat(name, lat)         do {} while (0)

#endif

} // namespace pos
