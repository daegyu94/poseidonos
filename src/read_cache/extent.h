#pragma once 

#include <memory>

#include "src/read_cache/list_head.h"
#include "src/read_cache/cache_const.h"
#include "src/lib/bitmap.h"

namespace pos {
/* recommended extent size: 128KB? 256KB? */
extern uint64_t extent_size;
extern uint64_t blocks_per_extent;

static constexpr uint64_t max_prefetch_size = 128 * 1024;

static inline uint64_t extent_start(uint64_t blk_addr) {
    return blk_addr - (blk_addr % blocks_per_extent);
}

static inline uint64_t extent_offset(uint64_t blk_addr) {
    return blk_addr % blocks_per_extent;
}

static inline uint64_t rba_start(uint64_t rba) {
    return rba - (rba % extent_size);
}

struct Extent {
    KeyType key;
    
    unsigned len; // number of blocks
    uintptr_t addr;

	struct list_head list; // list

    std::atomic<int> prefetch_in_progress;
    std::atomic<bool> memcpy_in_progress;
    
    BitMap *bitmap;
    uint64_t timestamp;

    Extent() = default; 

    Extent(KeyType key, uint64_t addr, int num_prefetch_in_progress) : 
        key(key), addr(addr), 
        prefetch_in_progress{num_prefetch_in_progress}, 
        memcpy_in_progress{false} {
            INIT_LIST_HEAD(&list);
            bitmap = new BitMap(blocks_per_extent);
        }

    ~Extent() {
        delete bitmap;
    }
};

struct RequestExtent {
    uint64_t blk_addr;
    unsigned len;

    RequestExtent(uint64_t blk_addr, unsigned len) : 
        blk_addr(blk_addr), len(len) { }
};

}; // namespace pos
