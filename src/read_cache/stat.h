#pragma once

#include <stdint.h>

namespace pos {
enum readcache_stat_name_t {
    RC_ADMIT_SUCC,
    RC_ADMIT_CONTAINED_SUCC,
    RC_ADMIT_LOCKED_FAILED,
    RC_ADMIT_UNALLOC_FAILED,
    
    RC_SINGLE_HIT,
    RC_SINGLE_MISS,

    RC_MERGED_HIT,
    RC_MERGED_MISS,
    RC_MERGED_PARTIAL_MISS,
    
    RC_UPDATE_SUCC,
    RC_EVICT_SUCC,
    RC_EVICT_FAILED,
    
    MAX_READCACHE_STAT,
};

struct readcache_stat {
    uint64_t cnts[MAX_READCACHE_STAT];
};

extern struct readcache_stat readcache_stat;
extern const char *readcache_stat_names[];

static inline void readcache_stat_inc(int stat_name, int tid, int cnt = 1)
{
    readcache_stat.cnts[stat_name] += cnt;
}

int readcache_stat_spawn_monitor(void);
} // namespace pos
