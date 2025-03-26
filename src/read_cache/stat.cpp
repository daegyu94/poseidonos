#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <unistd.h>

#include "src/read_cache/stat.h"
//#include "src/read_cache/read_cache.h"

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
    
    "RC_ISSUED_IO",
};

static int interval_sec = 1;
static bool overwrite_per_iter = true;
static const char *logfile_name = "/home/daegyu/prefetch-cache/log/readcache.log";

static uint64_t old_hit_cnt = 0;
static uint64_t old_miss_cnt = 0;

void readcache_log_stat(void) {
    std::ofstream logfile;

    if (overwrite_per_iter == false) {
        logfile.open(logfile_name, std::ios::out | std::ios::trunc);
        if (!logfile.is_open()) {
            std::cerr << "Failed to open readcache.log" << std::endl;
            return;
        }
    }
 
    int64_t before_read_cnt = 0, before_write_cnt = 0;
    int64_t diff_read_cnt = 0, diff_write_cnt = 0;
    int64_t total_cnt;

    while (true) {
        before_read_cnt = readcache_stat.read_cnt;
        before_write_cnt = readcache_stat.write_cnt;
        total_cnt = before_read_cnt + before_write_cnt;

        sleep(interval_sec);
        
        diff_read_cnt = readcache_stat.read_cnt- before_read_cnt;
        diff_write_cnt = readcache_stat.write_cnt - before_write_cnt;
        total_cnt = diff_read_cnt + diff_write_cnt;
        
        double write_ratio = (total_cnt > 0) ?
            (static_cast<double>(diff_write_cnt) / total_cnt) * 100.0 : 0.0;
        
        if (write_ratio >= 50.0) {
            readcache_stat.write_intensive = true;
        } else {
            readcache_stat.write_intensive = false;
        }

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
            logfile << std::setw(25) << readcache_stat_names[i] << "=" 
                << readcache_stat.cnts[i] << std::endl;
        }

        if (readcache_stat.cnts[RC_HIT]) {
            uint64_t hit_cnt = readcache_stat.cnts[RC_HIT];
            uint64_t miss_cnt = readcache_stat.cnts[RC_MISS];
            double hit_ratio = (1.0 * hit_cnt) / (hit_cnt + miss_cnt);
            logfile << std::setw(25) << "Hit ratio=" << hit_ratio << std::endl;
    
            uint64_t hit_cnt_diff = hit_cnt - old_hit_cnt;
            uint64_t miss_cnt_diff = miss_cnt - old_miss_cnt;
    
            double hit_ratio_per_sec = 0.0;
            if (hit_cnt_diff + miss_cnt_diff) {
                hit_ratio_per_sec = (1.0 * hit_cnt_diff) / 
                    (hit_cnt_diff + miss_cnt_diff);
            }
            logfile << std::setw(25) << "Hit ratio/sec=" << hit_ratio_per_sec << std::endl;

            old_hit_cnt = hit_cnt;
            old_miss_cnt = miss_cnt;
        }
            
        logfile << std::setw(25) << "Write intensive=" << readcache_stat.write_intensive << 
            "(" << write_ratio << " %)" << std::endl;
        logfile << std::setw(25) << "Pool used(MB)=" << readcache_stat.pool_used / (1 << 20) << std::endl;
        logfile << std::setw(25) << "Cache evict=" << readcache_stat.cache_evict << std::endl;

        /* Latency breakdown */
        uint64_t issued_io_cnt = readcache_stat.cnts[RC_ISSUED_IO];
        double lat_seg_alloc = elapsed_avg_us(BR_SEG_ALLOC, issued_io_cnt);
        double lat_seg_cache = elapsed_avg_us(BR_SEG_CACHE, issued_io_cnt);
        double lat_rba_lock = elapsed_avg_us(BR_RBA_LOCK, issued_io_cnt);
        double lat_issue_io = elapsed_avg_us(BR_ISSUE_IO, issued_io_cnt);
                
        logfile << "[Latency breakdown (us)]"
            << "\nSegment allocator: " << lat_seg_alloc
            << "\nSegment cache: " << lat_seg_cache 
            << "\nRba lock: " << lat_rba_lock
            << "\nIssue IO: " << lat_issue_io
            << "\n";
        
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

LatencyBreakdown br = {};

} // namespace pos
