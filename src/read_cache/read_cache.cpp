#include "src/include/memory.h"
#include "src/master_context/config_manager.h"
#include "src/logger/logger.h"
#include "src/spdk_wrapper/accel_engine_api.h"

#include "src/read_cache/read_cache.h"
#include "src/read_cache/stat.h"

#include <fstream>

namespace pos {
uint64_t extent_size = 0;
uint64_t blocks_per_extent = 0;

size_t getHugePageUsage() {
    size_t freeHugePages = 0;
    size_t totalHugePages = 0;
    ifstream meminfo("/proc/meminfo");
    string line;

    // "/proc/meminfo"에서 HugePages 관련 정보를 찾음
    while (getline(meminfo, line)) {
        if (line.find("HugePages_Free") != string::npos) {
            // HugePages_Free 값을 읽음
            sscanf(line.c_str(), "HugePages_Free: %zu", &freeHugePages);
        } else if (line.find("HugePages_Total") != string::npos) {
            // HugePages_Total 값을 읽음
            sscanf(line.c_str(), "HugePages_Total: %zu", &totalHugePages);
        }
    }

    return totalHugePages - freeHugePages;  // 사용 중인 Hugepage 수 반환
}

static std::string read_cache_str = "ReadCache";
void ReadCache::Initialize() {
    ConfigManager& configManager = *ConfigManagerSingleton::Instance();
    std::string module("read_cache");

    enabled_ = false;

    configManager.GetValue(module, "enable", &enabled_, CONFIG_TYPE_BOOL);
 
    testType_ = kNoTest;
    std::string testTypeStr;
#if 0
    configManager.GetValue(module, "test_type", &testTypeStr, 
            CONFIG_TYPE_STRING);
    
    auto testTypeIter = testTypeMap_.find(testTypeStr);
    if (testTypeIter != testTypeMap_.end()) {
        testType_ = testTypeIter->second;
        if (testType_ != kNoTest) {
            enabled_ = true;
        }
    }
#endif
    if (!enabled_) {
        return;
    }
    
    size_t cache_size_mb = 1024;
    size_t cache_size; 
    configManager.GetValue(module, "cache_size_mb", &cache_size_mb, 
            CONFIG_TYPE_UINT64);
    cache_size = cache_size_mb * (1UL << 20);
    
    std::string admissionPolicyStr;
    configManager.GetValue(module, "admission_policy", &admissionPolicyStr, 
            CONFIG_TYPE_STRING);
    int admissionPolicy = kPrefetchAdmission;
    auto iter2 = admissionPolicyMap_.find(admissionPolicyStr);
    if (iter2 != admissionPolicyMap_.end()) {
        admissionPolicy = iter2->second;
    } 
    admissionPolicy_ = admissionPolicy;

    size_t extent_size_kb = 128;
    configManager.GetValue(module, "extent_size_kb", &extent_size_kb, 
            CONFIG_TYPE_UINT64);
    if (admissionPolicy_ == kReadAdmission) {
        extent_size = 4 * 1024;
    } else {
        extent_size = extent_size_kb * 1024;
    }

    blocks_per_extent = extent_size / BLOCK_SIZE;
    
    assert(extent_size > 0 || blocks_per_extent > 0);

    size_t before_usage = getHugePageUsage();
#ifdef CONFIG_EXTENT_POOL
    memoryManager_ = MemoryManagerSingleton::Instance();

    BufferInfo info = {
        .owner = read_cache_str,
        .size = extent_size,
        .count = MAX_PENDING_IO * 3 / 2
    }; 
    bufferPool_ = memoryManager_->CreateBufferPool(info, 0);
    assert(bufferPool_ != nullptr);

    extentPool_ = new ExtentPool(cache_size, extent_size);
    max_num_buffers_ = cache_size / extent_size;
#else
    memoryManager_ = MemoryManagerSingleton::Instance();

    BufferInfo info = {
        .owner = read_cache_str,
        .size = extent_size,
        .count = cache_size / info.size
    }; 
    bufferPool_ = memoryManager_->CreateBufferPool(info, 0);
    assert(bufferPool_ != nullptr);
    
    max_num_buffers_ = info.count;
#endif
    size_t after_usage = getHugePageUsage();
    size_t usage_diff = after_usage - before_usage;
    printf("[INFO] %s: Hugepage diff=%lu (%lu, %lu))\n", __func__, 
            usage_diff, before_usage, after_usage);

    num_buffers_.store(0);

    /* frontend reactor (get/delete) + event reactor (put/evict) * 1.5x */
    //int num_shards = AccelEngineApi::GetReactorCount() * 3 / 2;
    int num_shards = 128;
    std::string policyStr;
    int policy = kFIFOFastEvictionPolicy;

    configManager.GetValue(module, "cache_policy", &policyStr, 
            CONFIG_TYPE_STRING);
    
    auto iter = cachePolicyMap_.find(policyStr);
    if (iter != cachePolicyMap_.end()) {
        policy = iter->second;
    }
    
    if (admissionPolicy_ == kReadAdmission) {
        policyStr = "LRUPolicy";
        policy = kLRUPolicy;
    }

    cache_ = new FixedSizedCache(max_num_buffers_, policy, num_shards);

    printf("[INFO] %s, Initialize ReadCache: policy=%s, "
            "cache_size_mb=%lu, max_num_buffers=%lu, extent_size=%lu, "
            "num_shards=%d\n", 
            __func__,
            policyStr.c_str(), 
            cache_size_mb, max_num_buffers_, extent_size, num_shards);

    readcache_stat_spawn_monitor();
}
} // namespace pos
