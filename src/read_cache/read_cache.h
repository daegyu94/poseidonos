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
constexpr int MAX_GET_BUFFER_RETRY_CNT = 1;

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
    
    void Put(int array_id, uint32_t volume_id, BlkAddr blk_addr, uintptr_t addr, 
            uint32_t valid_offset = 0, uint32_t valid_block_count = 0) {
        KeyType key(array_id, volume_id, blk_addr);
        ValueType value = new Extent(key, addr, valid_offset, valid_block_count); 

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
    
    int Get(int array_id, uint32_t volume_id, 
            std::pair<BlkAddr, bool> &blk_addr_p, uintptr_t &addr) {
        BlkAddr blk_addr = blk_addr_p.first;
        KeyType key(array_id, volume_id, extent_start(blk_addr));
        ValueType value = nullptr;
        RequestExtent request_extent(blk_addr, 1);
        uintptr_t inv_blk_addr = 0;

        int ret = cache_->Get(key, value, request_extent, inv_blk_addr, true); 
        if (ret > 0) {
            Extent *extent = (Extent *) value;
            uint32_t blk_addr_offset = extent_offset(blk_addr);
            if (!extent->bitmap->IsSetBit(blk_addr_offset)) {
                ret = -1;
            } else {
                addr = extent->addr + blk_addr_offset * BLOCK_SIZE;
            }
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

    void AdmitOrUpdate(int array_id, uint32_t volume_id, BlkAddr _blk_addr, 
            uint32_t block_count, 
            std::vector<std::pair<uintptr_t, bool>> &addr_p_vec, 
            std::vector<std::pair<BlkAddr, bool>> &blk_addr_p_vec) {
        BlkAddr blk_addr = _blk_addr;
        BlkAddr end_blk_addr = _blk_addr + block_count - 1;
        uint32_t num_remain_blocks = block_count;
        int vec_idx = 0;
        BlkAddr key_blk_addr_in_progress = -1;

        while (true) {
            uint32_t blk_addr_offset = extent_offset(blk_addr);
            uint32_t diff1 = blocks_per_extent - blk_addr_offset;
            uint32_t diff2 = end_blk_addr - blk_addr + 1;
            uint32_t count = diff1 > diff2 ? diff2 : diff1;
            uintptr_t inv_blk_addr = 0;
            
            BlkAddr cur_key_blk_addr = extent_start(blk_addr);
            KeyType key(array_id, volume_id, cur_key_blk_addr);
            ValueType value = nullptr;
            RequestExtent request_extent(blk_addr, count);
            
            /* bypass allocated new segment */
            if (key_blk_addr_in_progress == cur_key_blk_addr) {
                vec_idx += count;
                num_remain_blocks -= count;
                if (num_remain_blocks == 0) {
                    goto out;
                }
                continue;
            }

            /* write-through: should be succeeded for data consistency */
            cache_->Get(key, value, request_extent, inv_blk_addr, false);

            if (value) {
                /* update already cached data */
                Extent *extent = (Extent *) value;
                uintptr_t addr = extent->addr;
                
                blk_addr_p_vec.push_back(std::make_pair(key.blk_rba, true));
                
                for (uint32_t i = blk_addr_offset; i < blk_addr_offset + count; 
                        i++) {
                    extent->bitmap->SetBit(i);

                    addr_p_vec.insert(addr_p_vec.begin() + vec_idx, 
                            std::make_pair(addr + (i * BLOCK_SIZE), true));

                    rc_debug("hit: blk_addr=%lu, vec_idx=%d, addr=%lu, "
                            "block_count=(%u, %u, %u)\n",
                            blk_addr + i, vec_idx, 
                            addr_p_vec[vec_idx].first, 
                            block_count, num_remain_blocks, count);

                    vec_idx++;
                    num_remain_blocks--;
                    if (num_remain_blocks == 0) {
                        goto out;
                    }
                }
            } else {
                /* allocate buffer if possible and memcpy */
                int retry_cnt = 0;
                uintptr_t addr;

                while (retry_cnt++ < MAX_GET_BUFFER_RETRY_CNT) {
                    addr = TryGetBuffer();
                    if (addr) {
                        break;
                    }
                    Evict();
                }

                if (!addr) {
                    vec_idx += count;
                    goto alloc_failed;
                }

                Put(array_id, volume_id, cur_key_blk_addr, addr, 
                        blk_addr_offset, count);

                for (uint32_t i = blk_addr_offset; i < blk_addr_offset + count; 
                        i++) {
                    addr_p_vec.insert(addr_p_vec.begin() + vec_idx++, 
                            std::make_pair(addr + (i * BLOCK_SIZE), true));
                }

                blk_addr_p_vec.push_back(
                        std::make_pair(cur_key_blk_addr, true));

                key_blk_addr_in_progress = cur_key_blk_addr;

alloc_failed:
                rc_debug("miss: blk_addr=%lu, block_count=(%u, %u, %u)\n",
                        blk_addr, block_count, num_remain_blocks, count);
                num_remain_blocks -= count;
                if (num_remain_blocks == 0) {
                    goto out;
                }
            }

            blk_addr += count;
            assert(extent_offset(blk_addr) == 0); 
        }
out:
        return; 
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
        
        while (true) {
            uint32_t blk_addr_offset = extent_offset(blk_addr);
            uint32_t diff1 = blocks_per_extent - blk_addr_offset;
            uint32_t diff2 = end_blk_addr - blk_addr + 1;
            uint32_t count = diff1 > diff2 ? diff2 : diff1;
            uintptr_t inv_blk_addr = 0;

            KeyType key(array_id, volume_id, extent_start(blk_addr));
            ValueType value = nullptr;
            RequestExtent request_extent(blk_addr, count);

            /* write-through: should be succeeded for data consistency */
            cache_->Get(key, value, request_extent, inv_blk_addr, is_read);
            
            if (value) {
                Extent *extent = ((Extent *) value);
                uintptr_t addr = extent->addr;
                bool is_inv = inv_blk_addr ? true : false;
                
                blk_addr_p_vec.push_back(std::make_pair(key.blk_rba, is_inv));

                for (uint32_t i = blk_addr_offset; i < blocks_per_extent; i++) {
                    bool is_valid = extent->bitmap->IsSetBit(i);
                    if (is_valid) {
                        addrs.insert(addrs.begin() + vec_idx, 
                                std::make_pair(addr + (i * BLOCK_SIZE), 
                                    is_valid));
                        num_found++;
                    }
                    vec_idx++;

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
    
    std::map<std::string, int> cachePolicyMap_ = {
        {"FIFOPolicy", kFIFOPolicy},
        {"FIFOFastEvictionPolicy", kFIFOFastEvictionPolicy},
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
