#pragma once

#include "src/read_cache/extent_cache.h"
#include "src/read_cache/cache_const.h"

namespace pos {
class FixedSizedCache {
public:
    FixedSizedCache(size_t max_size, int policy, int num_shards) : 
        max_size_{max_size}, policy_(policy), num_shards_(num_shards)
    {
        locks_ = new pthread_rwlock_t[num_shards_];
        caches_ = new ICachePolicy *[num_shards_];
        
        request_cnt_ = 0;

        for (int i = 0; i < num_shards_; i++) {
            pthread_rwlock_init(&locks_[i], NULL);

            switch (policy_) {
                case kFIFOPolicy:
                    caches_[i] = new ExtentCache(max_size_ / num_shards_, 
                            kFIFOPolicy);
                    break;
                case kFIFOFastEvictionPolicy:
                    caches_[i] = new ExtentCache(max_size_ / num_shards_, 
                            kFIFOFastEvictionPolicy);
                    break;
                default:
                    throw std::runtime_error("wrong cache policy" + 
                            std::to_string(policy_));
            }
        }
        
        timestamps_ = new uint64_t[num_shards_]();
    }

    ~FixedSizedCache() {
        for (int i = 0; i < num_shards_; i++) {
            delete caches_[i];
            pthread_rwlock_destroy(&locks_[i]);
        }
        delete[] caches_;
        delete[] locks_;
        delete[] timestamps_;
    }
    
    /* blk_rba must be extent aligned, except for update extent meta */

    void Put(const KeyType &key, const ValueType &value) {
        int id = GetShardId(key.blk_rba);
        
        pthread_rwlock_wrlock(&locks_[id]);
        caches_[id]->Put(key, value);
        pthread_rwlock_unlock(&locks_[id]);
    }

    bool Contain(const KeyType &key) {
        int id = GetShardId(key.blk_rba);
         
        if (policy_ == kFIFOPolicy) {
            pthread_rwlock_rdlock(&locks_[id]);
        } else {
            pthread_rwlock_wrlock(&locks_[id]);
        }
        bool ret = caches_[id]->Contain(key);
        pthread_rwlock_unlock(&locks_[id]);
        
        return ret;
    }

    void ClearInProgress(const KeyType &key, int in_progress_type) {
        int id = GetShardId(key.blk_rba);

        pthread_rwlock_rdlock(&locks_[id]);
        caches_[id]->ClearInProgress(key, in_progress_type);
        pthread_rwlock_unlock(&locks_[id]);
    }

    /* it should be success for write, okay to be failed for read */
    int Get(const KeyType &key, ValueType &value, 
            const RequestExtent &request_extent, 
            uintptr_t &inv_blk_addr, bool is_read) {
        int id = GetShardId(key.blk_rba);
        constexpr int max_retry_cnt = 3;
        int retry_cnt = 0;
        int ret;

    retry_cache_op: 
        if (is_read && (retry_cnt++ == max_retry_cnt)) {
                ret = 0;
                goto out;
        }

        if (policy_ == kFIFOPolicy || policy_ == kFIFOFastEvictionPolicy) {
            pthread_rwlock_rdlock(&locks_[id]);
        } else {
            pthread_rwlock_wrlock(&locks_[id]);
        }
        ret = caches_[id]->Get(key, value, request_extent, inv_blk_addr);
        pthread_rwlock_unlock(&locks_[id]);
        if (ret < 0) {
            goto retry_cache_op;
        }

    out:
        assert(ret >= 0);

        return ret;
    }

    /* it should be success by retrying */
    bool Delete(const KeyType &key, ValueType &value) {
        int id = GetShardId(key.blk_rba);
    retry_cache_op:
        pthread_rwlock_wrlock(&locks_[id]);
        int ret = caches_[id]->Delete(key, value);
        pthread_rwlock_unlock(&locks_[id]);
        if (ret == -1) 
            goto retry_cache_op;

        return ret;
    }
 
    void Evict(ValueType &value) {
        int id = GetShardId(request_cnt_++);
        
        pthread_rwlock_wrlock(&locks_[id]);
        caches_[id]->Evict(value);
        pthread_rwlock_unlock(&locks_[id]);
    }
   
    size_t Size(int id) {
        return caches_[id]->Size();
    }
    
    double Util(int id) {
        return caches_[id]->Util();
    }

    void Print(int id) {
        caches_[id]->Print();
    }
    
private:
    int GetShardId(const uint64_t blk_rba) {
        return H(blk_rba) % num_shards_;
    }

    uint64_t H(const uint64_t &key) {
        size_t seed = 0xc70f6907UL;
        return std::_Hash_bytes((void *) &key, sizeof(uint64_t), seed);
    }

    size_t max_size_;
    int policy_;
    int num_shards_;
    ICachePolicy **caches_;
    pthread_rwlock_t *locks_;
    
    uint64_t request_cnt_;
    uint64_t *timestamps_;
};

} // namespace pos
