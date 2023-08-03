#include "src/include/memory.h"
#include "src/master_context/config_manager.h"
#include "src/logger/logger.h"
#include "src/spdk_wrapper/accel_engine_api.h"

#include "src/read_cache/read_cache.h"

namespace pos {
uint64_t extent_size = 0;
uint64_t blocks_per_extent = 0;

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
        
    size_t extent_size_kb = 128;
    configManager.GetValue(module, "extent_size_kb", &extent_size_kb, 
            CONFIG_TYPE_UINT64);
    extent_size = extent_size_kb * 1024;
    blocks_per_extent = extent_size / BLOCK_SIZE;
    
    assert(extent_size > 0 || blocks_per_extent > 0);

    memoryManager_ = MemoryManagerSingleton::Instance();
    
    BufferInfo info = {
        .owner = read_cache_str,
        .size = extent_size,
        .count = cache_size / info.size
    };
    
    //bufferPool_ = memoryManager_->CreateBufferPool(info);
    bufferPool_ = memoryManager_->CreateBufferPool(info, 0);
    assert(bufferPool_ != nullptr);
    
    max_num_buffers_ = info.count;
    num_buffers_.store(0);

    /* frontend reactor (get/delete) + event reactor (put/evict) * 1.5x */
    int num_shards = AccelEngineApi::GetReactorCount() * 3 / 2;
    std::string policyStr;
    int policy = kFIFOFastEvictionPolicy;

    configManager.GetValue(module, "cache_policy", &policyStr, 
            CONFIG_TYPE_STRING);
    
    auto iter = cachePolicyMap_.find(policyStr);
    if (iter != cachePolicyMap_.end()) {
        policy = iter->second;
    }
   
    cache_ = new FixedSizedCache(max_num_buffers_, policy, num_shards);

    printf("Initialize ReadCache: policy=%s, "
            "cache_size_mb=%lu, max_num_buffers=%lu, extent_size=%lu, "
            "num_shards=%d\n", 
            policyStr.c_str(), 
            cache_size_mb, max_num_buffers_, extent_size, num_shards);
}
} // namespace pos
