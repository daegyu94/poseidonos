#pragma once

#include <cassert>

#include "i_cache_policy.h"
#include "extent.h"

namespace pos {
class ExtentCache : public ICachePolicy {
public:
    /* inner class in each cache policy */
    class Bucket {
    public:
        size_t hash;
        ValueType value;
        struct hlist_node hnode; // hash table

        Bucket() = default;
        Bucket(const size_t hash, const ValueType value) : 
            hash(hash), value(value) { }

        void *operator new(size_t size) {
            void *ret;
            if (posix_memalign(&ret, 64, size)) ret = nullptr;
            return ret;
        }

        void *operator new[](size_t size) {
            void *ret;
            if (posix_memalign(&ret, 64, size)) ret = nullptr;
            return ret;
        }
    };
    
    class ValueList {
    public:
        ValueList(int policy) : policy_(policy), request_cnt_(0) {
            INIT_LIST_HEAD(&list_);
            INIT_LIST_HEAD(&mostly_used_list_);
        }

        ~ValueList(void) { }
        
        /* insert or update */
        void Insert(Extent *extent) {
            list_add_tail(&extent->list, &list_);
        }
        
        void Delete(Extent *extent) {
            list_del(&extent->list);
        }
        
        void Evict(ValueType &evicted_value) {
            Extent *extent;

            list_for_each_entry(extent, &list_, list) {
                if (extent->memcpy_in_progress.load())
                    continue;

                evicted_value = extent;
                list_del(&extent->list);
                return;
            }
        }
         
        void Print() {
#if 0
            struct list_head *cur_list;
            Extent *extent;
            
            printf("********* Freq list ********\n");
            for (int i = 0; i < num_bins_; i++) {
                printf("[%2d, %3d]", i, i * range_);
                cur_list = &lists_[i];
                list_for_each_entry(extent, cur_list, list) {
                    //printf("<->(lcn=%lu, util=%d)", 
                            //extent->lcn, extent->meta.GetUtil());
                    printf("<->(blk_addr=%lu)", extent->blk_addr); 
                }
                printf("\n");
            }
            printf("****************************\n");
#endif
        }

    private:
        int policy_;
        uint64_t request_cnt_;
        struct list_head list_;
        struct list_head mostly_used_list_;
    };


    ExtentCache(size_t capacity, int policy) : 
        capacity_(capacity), num_buckets_(capacity * 75 / 100), size_(0), 
        policy_(policy) {
        hheads_ = new struct hlist_head[num_buckets_];
        for (unsigned i = 0; i < num_buckets_; i++) {
            INIT_HLIST_HEAD(&hheads_[i]);
        }
        
        value_list_ = new ValueList(policy_);

        printf("%s: num_buckets=%u, size(KB)=%lu\n", 
                __func__, num_buckets_, sizeof(Bucket) * num_buckets_ / 1024);   
    }

    ~ExtentCache() {
        delete[] hheads_;
        delete value_list_;
    }

    uintptr_t _Get(const KeyType &key) {
        Bucket *cur_bucket;
        size_t hash = H(key); 
        uint32_t bkt_id = hash % num_buckets_;
        uintptr_t ret_addr = 0;

        hlist_for_each_entry(cur_bucket, &hheads_[bkt_id], hnode) {
            if (cur_bucket->hash == hash && 
                    ((Extent *) cur_bucket->value)->key == key) {
                Extent *extent = (Extent *) cur_bucket->value;

                if (extent->memcpy_in_progress.load()) {
                    ret_addr = (uintptr_t) -1;
                    break;
                }

                ret_addr = (uintptr_t) extent->addr;
                if (policy_ == kFIFOFastEvictionPolicy) {
                    extent->bitmap->ResetBitmap();
                }
                break;
            }
        }
        return ret_addr;
    }

    uintptr_t Put(const KeyType &key, const ValueType &value) override {
        uintptr_t ret_addr = _Get(key);
        if (ret_addr) {
            return ret_addr;
            //assert(0); 
        }

        size_t hash = H(key);
        uint32_t bkt_id = hash % num_buckets_;
        
        Bucket *new_bucket = new Bucket(hash, value);
        /* add to hashtable */
        hlist_add_head(&new_bucket->hnode, &hheads_[bkt_id]);
        
        // size_++;
        assert(value);
        
        value_list_->Insert((Extent *) value); 
        ((Extent *) value)->memcpy_in_progress.store(true);

        return ret_addr;
    }

    void ClearInProgress(const KeyType &key, int in_progress_type) override {
        Bucket *cur_bucket;
        size_t hash = H(key);
        uint32_t bkt_id = hash % num_buckets_;

        hlist_for_each_entry(cur_bucket, &hheads_[bkt_id], hnode) {
            if (cur_bucket->hash == hash && 
                    ((Extent *) cur_bucket->value)->key == key) {
                Extent *extent = ((Extent *) cur_bucket->value);
                extent->memcpy_in_progress.store(false);
                return;
            }
        }
        if (policy_ == kFIFOPolicy) {
            assert(0);
        }
    }
    
    int Get(const KeyType &key, ValueType &value, 
            const RequestExtent &request_extent, 
            uintptr_t &inv_blk_addr) override {
        Bucket *cur_bucket;
        size_t hash = H(key);
        uint32_t bkt_id = hash % num_buckets_;
        int ret = 0;
        
        hlist_for_each_entry(cur_bucket, &hheads_[bkt_id], hnode) {
            if (cur_bucket->hash == hash && 
                    ((Extent *) cur_bucket->value)->key == key) {
                Extent *extent = ((Extent *) cur_bucket->value);

                if (extent->memcpy_in_progress.load()) {
                    ret = -1;
                    break;
                }

                value = cur_bucket->value;
                extent->memcpy_in_progress.store(true);
                ret = 1;

                /* prevent waste (amplification) => eviction while in list */
                if (policy_ == kFIFOFastEvictionPolicy) {
                    int util = 100 * extent->bitmap->GetNumBitsSet() / 
                        blocks_per_extent;
                    if (util > 70 || (util && util < 30)) {
                        inv_blk_addr = key.blk_rba;
                    }
                }

                break;
            }
        }
        return ret;
    }
    
    /* XXX: we delete the extent after memcpy if meets invalidation condition */
    int Delete(const KeyType &key, ValueType &value) override {
        Bucket *cur_bucket;
        struct hlist_node *tmp;
        size_t hash = H(key);
        uint32_t bkt_id = hash % num_buckets_;
        int ret = 0;
        
        hlist_for_each_entry_safe(cur_bucket, tmp, &hheads_[bkt_id], hnode) {
            if (cur_bucket->hash == hash && 
                    ((Extent *) cur_bucket->value)->key == key) {
                value = cur_bucket->value;

                hlist_del(&cur_bucket->hnode);
                
                value_list_->Delete((Extent *) value); 
                
                // size_--;
                ret = 1;
                delete cur_bucket;
                break;
            }
        }
        
        return ret;
    }
    
    /* extent is already detached from list */
    int _Delete(const KeyType &key) {
        Bucket *cur_bucket;
        struct hlist_node *tmp;
        size_t hash = H(key);
        uint32_t bkt_id = hash % num_buckets_;
        int ret = 0;
        
        hlist_for_each_entry_safe(cur_bucket, tmp, &hheads_[bkt_id], hnode) {
            if (cur_bucket->hash == hash && 
                    ((Extent *) cur_bucket->value)->key == key) {
                hlist_del(&cur_bucket->hnode);
                
                assert(((Extent *) cur_bucket->value)->memcpy_in_progress == 0);
                // size_--;
                ret = 1;
                delete cur_bucket;
                break;
            }
        }
        
        assert(ret);

        return ret;
    }  

    void Evict(ValueType &evicted_value) override {
        value_list_->Evict(evicted_value);
        if (!evicted_value)
            return;

        Extent *extent = (Extent *) evicted_value;
        KeyType key = extent->key;
        /* remove from hashtable */
        _Delete(key);
    }

    void Print() override {
        value_list_->Print(); 
    }

private:
    const size_t capacity_;
    unsigned num_buckets_;
    size_t size_;
    int policy_;
    struct hlist_head *hheads_;
    
    ValueList *value_list_;
    
    uint64_t Mix(uint64_t a, uint64_t b, uint64_t c) {
        a = a - b; a = a - c; a = a ^ (c >> 43);
        b = b - c; b = b - a; b = b ^ (a << 9);
        c = c - a; c = c - b; c = c ^ (b >> 8);
        a = a - b; a = a - c; a = a ^ (c >> 38);
        b = b - c; b = b - a; b = b ^ (a << 23);
        c = c - a; c = c - b; c = c ^ (b >> 5);
        a = a - b; a = a - c; a = a ^ (c >> 35);
        b = b - c; b = b - a; b = b ^ (a << 49);
        c = c - a; c = c - b; c = c ^ (b >> 11);
        a = a - b; a = a - c; a = a ^ (c >> 12);
        b = b - c; b = b - a; b = b ^ (a << 18);
        c = c - a; c = c - b; c = c ^ (b >> 22);
        return c;
    }

    uint64_t H(const KeyType &key) {
        return Mix(key.array_id, key.volume_id, key.blk_rba);
    }
};
} // namespace pos
