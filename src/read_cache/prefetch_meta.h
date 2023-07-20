#pragma once

#include "src/lib/block_alignment.h"
#include "src/network/nvmf_target.h"
#include "src/io/general_io/translator.h"
#include "src/io/general_io/rba_state_service.h"
#include "src/allocator_service/allocator_service.h"
#include "src/read_cache/extent.h"

namespace pos {
struct PrefetchMeta {
    int arrayId;
    uint32_t volumeId;
    uint64_t rba;

    PrefetchMeta(int a, uint32_t n, uint64_t r) {
        arrayId = a; /* subsys_id -> arr_id */
        volumeId = n; /* volume idx starts from 0 */ 
        rba = r;
    }
};
using PrefetchMetaSmartPtr = std::shared_ptr<PrefetchMeta>;
} // namespace pos
