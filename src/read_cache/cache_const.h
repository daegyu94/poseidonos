#pragma once

namespace pos {
enum KeyState {
    NONE = 0U,
    INVALID = -1U,
    TOMBSTONE = -2U,
};

enum cachePolicy {
    kFIFOPolicy = 0,
    kFIFOFastEvictionPolicy,
    kMaxPolicy
};

enum in_progress_type {
    kPrefetchInProgress = 0,
    kMemcpyInProgress
};

struct LongKey {
    int array_id;
    uint32_t volume_id;
    uint64_t blk_rba;

    LongKey(int k1, uint32_t k2, uint64_t k3): 
        array_id(k1), volume_id(k2), blk_rba(k3) {}

    bool operator==(const LongKey& other) const {
        return array_id == other.array_id && 
            volume_id == other.volume_id && 
            blk_rba == other.blk_rba;
    }
};

using KeyType = LongKey; // uint64_t;
using ValueType = void *;
} // namespace pos
