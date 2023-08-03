#pragma once

#include "src/lib/singleton.h"
#include "src/include/address_type.h"
#include "src/resource_manager/buffer_pool.h"
#include "src/resource_manager/memory_manager.h"
#include "src/read_cache/fixed_sized_cache.h"

#include <air/Air.h>

//#define READ_CACHE_DEBUG
#ifdef READ_CACHE_DEBUG
#define rc_debug(str, ...) printf("%s: " str, __func__, __VA_ARGS__)
#else 
#define rc_debug(str, ...) do {} while (0)  
#endif

namespace pos {
enum {
    kNoTest = 0,
    kGrpcOnly,
    kPrefetchOnly,
    kCacheOpOnly,
};

class ReadCache {
public:
    ReadCache(void) { }
    ~ReadCache(void) {
        if (enabled_) {
            memoryManager_->DeleteBufferPool(bufferPool_);
            delete cache_;
        }
    }
    
    void Initialize();
    
    void Put(int array_id, uint32_t volume_id, BlkAddr blk_addr, uintptr_t addr) {
        KeyType key(array_id, volume_id, blk_addr);
        ValueType value = new Extent(key, addr, 
                extent_size > max_prefetch_size ? 
                extent_size / max_prefetch_size : 1);

        cache_->Put(key, value);
    }
    
    bool Contain(int array_id, uint32_t volume_id, BlkAddr blk_addr) {
        KeyType key(array_id, volume_id, blk_addr);

        return cache_->Contain(key);
    }
 
    void ClearInProgress(int array_id, uint32_t volume_id, BlkAddr blk_addr, 
            int in_progress_type) {
        KeyType key(array_id, volume_id, extent_start(blk_addr));
        cache_->ClearInProgress(key, in_progress_type);
    }
    
    bool Get(int array_id, uint32_t volume_id, 
            std::pair<BlkAddr, bool> &blk_addr_p, uintptr_t &addr) {
        BlkAddr blk_addr = blk_addr_p.first;
        KeyType key(array_id, volume_id, extent_start(blk_addr));
        ValueType value = nullptr;
        RequestExtent request_extent(blk_addr, 1);
        uintptr_t inv_blk_addr = 0;
        bool is_buffer_util_high = is_buffer_util_high_;

        int ret = cache_->Get(key, value, request_extent, inv_blk_addr, true); 
        if (ret > 0)
            addr = ((Extent *) value)->addr + 
                extent_offset(blk_addr) * BLOCK_SIZE;

        if (inv_blk_addr && is_buffer_util_high) {
            blk_addr_p.second = true;
        }
        
        return ret;
    }
    
    bool Delete(int array_id, uint32_t volume_id, BlkAddr blk_addr, 
            uintptr_t &addr) {
        KeyType key(array_id, volume_id, extent_start(blk_addr));
        ValueType value = nullptr;

        bool ret = cache_->Delete(key, value);
        if (ret) {
            ReturnBuffer(((Extent *) value)->addr); 
            delete (Extent *) value;
            
            airlog("CNT_ReadCacheBuffer", "succ_evict", 0, 1); 
        }

        return ret;
    }

    uint32_t Scan(int array_id, uint32_t volume_id, BlkAddr _blk_addr, 
            uint32_t block_count, std::vector<std::pair<uintptr_t, bool>> &addrs, 
            std::vector<std::pair<BlkAddr, bool>> &blk_addr_p_vec, 
            bool is_read = false) {
        BlkAddr blk_addr = _blk_addr;
        BlkAddr end_blk_addr = _blk_addr + block_count - 1;
        uint32_t num_remain_blocks = block_count;
        uint32_t num_found = 0;
        int vec_idx = 0;
        bool is_buffer_util_high = is_buffer_util_high_;
        
        while (true) {
            uint32_t diff1 = blocks_per_extent - extent_offset(blk_addr);
            uint32_t diff2 = end_blk_addr - blk_addr + 1;
            uint32_t count = diff1 > diff2 ? diff2 : diff1;
            uintptr_t inv_blk_addr = 0;
            bool is_inv = false;

            KeyType key(array_id, volume_id, extent_start(blk_addr));
            ValueType value = nullptr;
            RequestExtent request_extent(blk_addr, count);

            /* write-through: should be succeeded for data consistency */
            cache_->Get(key, value, request_extent, inv_blk_addr, is_read);
            
            if (value) {
                if (inv_blk_addr && is_buffer_util_high) {
                    is_inv = true;
                }
                blk_addr_p_vec.push_back(std::make_pair(key.blk_rba, is_inv));

                uintptr_t addr = ((Extent *) value)->addr;
                for (uint64_t i = extent_offset(blk_addr); i < blocks_per_extent; 
                        i++) {
                    addrs.insert(addrs.begin() + vec_idx++, 
                            std::make_pair(addr + (i * BLOCK_SIZE), is_inv));
                    num_found++;
                    rc_debug("hit: blk_addr=%lu, vec_idx=%d, addr=%lu, "
                            "block_count=(%u, %u, %u)\n",
                            blk_addr + i, vec_idx - 1, addrs[vec_idx - 1].first, 
                            block_count, num_remain_blocks, count);

                    num_remain_blocks--;
                    if (num_remain_blocks == 0) {
                        goto out;
                    }
                }
            } else {
                rc_debug("miss: blk_addr=%lu, block_count=(%u, %u, %u)\n",
                        blk_addr, block_count, num_remain_blocks, count);
                vec_idx += count;
                num_remain_blocks -= count;
                if (num_remain_blocks == 0) {
                    goto out;
                }
            }
            blk_addr += count;
            assert(extent_offset(blk_addr) == 0); 
        }
out:
        return num_found;
    }
    
    /* XXX: need bg evictor to secure free buffer? */
    void Evict(void) {
        ValueType value = nullptr;

        cache_->Evict(value);
        if (!value) {
            airlog("CNT_ReadCacheBuffer", "failed_evict", 0, 1);
            return;
        }
        
        //Extent *ext = (Extent *) value;
        //printf("Evict extent: blk_addr=%lu, len=%u, addr=%lu, num_set=%u\n", 
        //        ext->blk_addr, ext->len, ext->addr, ext->meta.num_set);

        ReturnBuffer(((Extent *) value)->addr);

        delete (Extent *) value;

        airlog("CNT_ReadCacheBuffer", "succ_evict", 0, 1);
    }

    uintptr_t TryGetBuffer(void) {
        uintptr_t ret = (uintptr_t) bufferPool_->TryGetBuffer();
        if (ret) {
            num_buffers_.fetch_add(1);
            airlog("CNT_ReadCacheBuffer", "pool", 0, extent_size);
        }
        return ret;
    }

    void ReturnBuffer(uintptr_t addr) {
        num_buffers_.fetch_sub(1);
        bufferPool_->ReturnBuffer((void *) addr); 
        airlog("CNT_ReadCacheBuffer", "pool", 0, -extent_size);
    }

    BufferPool *GetBufferPool(void) {
        return bufferPool_;
    } 
    
    bool IsEnabled(void) const {
        return enabled_;
    }
    
    bool IsEnabledPrefetch(void) const {
        if (testType_ == kGrpcOnly || testType_ == kCacheOpOnly)
            return false;
        else
            return true;
    }

    bool IsEnabledCheckCache(void) const {
        return testType_ == kNoTest || testType_ == kCacheOpOnly;
    }
    
    void UpdateBufferUtil(void) {
        /* TODO: condition for fifo fast eviction */
        //if (0) {
        //    return;
        //}

        if (request_cnt_++ % 100) {
            return;
        }

        unsigned int util = (100UL * num_buffers_.load()) / max_num_buffers_;
        if (util > buffer_util_threshold_) {
            is_buffer_util_high_ = true;
        } else {
            is_buffer_util_high_ = false;
        }
    }

private:
    /* 
     * TODO: read cache option 
     * on/off: enabled
     * policy: inclusive, exclusive, hybrid (not destructive exclusive)
     *
     */
    bool enabled_;
    int testType_;
    
    std::map<std::string, int> cachePolicyMap_ = {
        {"FIFOPolicy", kFIFOPolicy},
        {"FIFOFastEvictionPolicy", kFIFOFastEvictionPolicy},
        {"DemotionPolicy", kDemotionPolicy}
    };

    std::map<std::string, int> testTypeMap_ = {
        {"no_test", kNoTest},
        {"grpc_only", kGrpcOnly},
        {"prefetch_only", kPrefetchOnly},
        {"cache_op_only", kCacheOpOnly}
    };

    /* memory pool for cached block */
    MemoryManager *memoryManager_;
    BufferPool *bufferPool_;
    size_t max_num_buffers_;
    std::atomic<size_t> num_buffers_;
    unsigned int buffer_util_threshold_ = 95;
    bool is_buffer_util_high_ = false;
    uint64_t request_cnt_ = 0;

    FixedSizedCache *cache_;
};

using ReadCacheSingleton = SimpleSingleton<ReadCache>;
} // namespace pos
