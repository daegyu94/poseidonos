#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <unistd.h>

#include "src/read_cache/stat.h"

namespace pos {
struct readcache_stat readcache_stat = {};

const char* readcache_stat_names[MAX_READCACHE_STAT] = {
    "RC_ADMIT_SUCC",
    "RC_ADMIT_CONTAINED_SUCC",
    "RC_ADMIT_LOCKED_FAILED",
    "RC_ADMIT_UNALLOC_FAILED",

    "RC_HIT",
    "RC_MISS",

    "RC_SINGLE_HIT",
    "RC_SINGLE_MISS",

    "RC_MERGED_HIT",
    "RC_MERGED_MISS",
    "RC_MERGED_PARTIAL_MISS",

    "RC_UPDATE_SUCC",
    "RC_EVICT_SUCC",
    "RC_EVICT_FAILED",
};

static int interval_sec = 3;
static bool overwrite_per_iter = true;
static const char *logfile_name = "/var/log/pos/readcache.log";

void readcache_log_stat(void) {
    std::ofstream logfile;

    if (overwrite_per_iter == false) {
        logfile.open(logfile_name, std::ios::out | std::ios::trunc);
        if (!logfile.is_open()) {
            std::cerr << "Failed to open readcache.log" << std::endl;
            return;
        }
    }
 
    while (true) {
        sleep(interval_sec);
        
        if (overwrite_per_iter == true) {
            logfile.open(logfile_name, std::ios::out | std::ios::trunc);
            if (!logfile.is_open()) {
                std::cerr << "Failed to open " << logfile_name << std::endl;
                continue;
            }
        }

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logfile << "[ ReadCache Stat ] " << std::ctime(&now);

        for (int i = 0; i < MAX_READCACHE_STAT; i++) {
            logfile << std::setw(25) << readcache_stat_names[i] << "=" << readcache_stat.cnts[i] << std::endl;
        }

        if (readcache_stat.cnts[RC_HIT]) {
            uint64_t hit_cnt = readcache_stat.cnts[RC_HIT];
            uint64_t miss_cnt = readcache_stat.cnts[RC_MISS];
            double hit_ratio = (1.0 * hit_cnt) / (hit_cnt + miss_cnt);
            logfile << std::setw(25) << "Hit ratio=" << hit_ratio << std::endl;

        }

        logfile << std::endl;
        
        if (overwrite_per_iter == true) { 
            logfile.close();
        }
    }

    if (logfile.is_open()) {
        logfile.close();
    }
}

int readcache_stat_spawn_monitor(void) {
    std::thread thd(readcache_log_stat);
    
    thd.detach();

    return 0;
}
} // namespace pos
