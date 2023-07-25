#pragma once

#include "src/read_cache/prefetch_meta.h"

#include "src/event_scheduler/callback.h"
#include "src/event_scheduler/event.h"

#include <atomic>

namespace pos {
class PrefetchDispatcher {
public:
	PrefetchDispatcher();
	~PrefetchDispatcher();
    
    bool Do(PrefetchMetaSmartPtr);

    bool FrontendIO(PrefetchMetaSmartPtr);
    void IssueFrontendIO(PrefetchMeta *, uintptr_t, size_t);
    
private:
    uint64_t request_cnt_;
    uint64_t reactor_cnt_;
    std::vector<uint32_t> event_reactor_cpu_vec_;
};
} // namespace pos
