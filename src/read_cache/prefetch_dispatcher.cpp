#include "src/read_cache/prefetch_dispatcher.h"

#include "src/bio/ubio.h"
#include "src/lib/block_alignment.h"
#include "src/io/general_io/translator.h"
#include "src/io/general_io/rba_state_service.h"
#include "src/io_scheduler/io_dispatcher.h"
#include "src/event_scheduler/event_scheduler.h"
#include "src/read_cache/read_cache.h"
#include "src/logger/logger.h"

#include "src/spdk_wrapper/accel_engine_api.h"
#include "src/spdk_wrapper/event_framework_api.h"
#include "src/io/frontend_io/unvmf_io_handler.h"

#include <air/Air.h>

//#define PREFETCH_DISPATCHER_DEBUG
#ifdef PREFETCH_DISPATCHER_DEBUG
#define pd_debug(str, ...) printf("%s: " str, __func__, __VA_ARGS__)
#else 
#define pd_debug(str, ...) do {} while (0)  
#endif

#define PREFETCH_EID 6789

namespace pos {
PrefetchDispatcher::PrefetchDispatcher(void) { 
    request_cnt_ = 0;
    reactor_cnt_ = AccelEngineApi::GetReactorCount();
    
    AffinityManager* affinity_manager = pos::AffinityManagerSingleton::Instance();
    cpu_set_t event_reactor_cpu_set = 
        affinity_manager->GetCpuSet(CoreType::EVENT_REACTOR);

    for (uint32_t cpu = 0; cpu < affinity_manager->GetTotalCore(); cpu++) {
        if (CPU_ISSET(cpu, &event_reactor_cpu_set)) {
            //printf("event_reactor_core=%u\n", cpu);
            event_reactor_cpu_vec_.push_back(cpu);
        }    
    }
}

PrefetchDispatcher::~PrefetchDispatcher(void) { }

bool LockRba(int array_id, uint32_t volume_id, uint64_t rba) {
    RBAStateManager *rbaStateManager = 
        RBAStateServiceSingleton::Instance()->GetRBAStateManager(array_id);
    return rbaStateManager->BulkAcquireOwnership(volume_id, 
            ChangeByteToBlock(rba), blocks_per_extent);
}

void UnlockRba(int array_id, uint32_t volume_id, uint64_t rba, size_t len) {
    RBAStateManager *rbaStateManager = 
        RBAStateServiceSingleton::Instance()->GetRBAStateManager(array_id);
    rbaStateManager->BulkReleaseOwnership(volume_id, ChangeByteToBlock(rba), 
            len);
}

bool PrefetchDispatcher::Do(PrefetchMetaSmartPtr meta) {
    return FrontendIO(meta);
}

static void dummy_spdk_call(void* arg1, void* arg2) { }

static void frontend_io_complete(struct pos_io* posIo, int status) {
    PrefetchMeta *meta = (PrefetchMeta *) posIo->context;
    int array_id = meta->arrayId;
    int volume_id = meta->volumeId;
    uint64_t rba = meta->rba;
    uint64_t start_rba = rba % extent_size ? rba_start(rba) : rba;
    
    ReadCacheSingleton::Instance()->ClearInProgress(array_id, 
            volume_id, ChangeByteToBlock(start_rba), kPrefetchInProgress);
    
    UnlockRba(array_id, volume_id, rba, posIo->length / BLOCK_SIZE);
    
    airlog("CNT_ReadCache", "succ_admit", 0, 1); 
    
    pd_debug("array_id=%d, volume_id=%d, rba=%lu, addr=%lu, len=%lu\n", 
            array_id, volume_id, rba, (uintptr_t) posIo->iov->iov_base, 
            posIo->length);

    delete (PrefetchMeta *) posIo->context;
    delete posIo->iov;
    delete posIo;
}

static void frontend_io_submit(void *arg1, void *arg2) {
    struct pos_io *posIo = static_cast<struct pos_io *>(arg1);
    uint64_t core = reinterpret_cast<uint64_t>(arg2);
    
    UNVMfSubmitHandler(posIo);

    auto eventFrameworkApi = EventFrameworkApiSingleton::Instance();
    eventFrameworkApi->SendSpdkEvent(core, 
            dummy_spdk_call, nullptr, nullptr);
}

/* XXX: don't need for data path, use for detect prefetch io */
static char arrayName[4] = "77"; // char arrayName[32] = "POSArray"; 
void PrefetchDispatcher::IssueFrontendIO(PrefetchMeta *meta, uintptr_t addr, 
        size_t size) {
    int array_id = meta->arrayId;
    int volume_id = meta->volumeId;
    uint64_t rba = meta->rba;

    struct pos_io *posIo = new pos_io;
    struct iovec *iov = new iovec; 
    iov->iov_base = (char *) addr;

    posIo->ioType = IO_TYPE::READ;
    posIo->volume_id = volume_id;
    posIo->iov = iov;
    posIo->iovcnt = 1; /* single contiguous buffer */
    posIo->length = size;
    posIo->offset = rba;
    posIo->context = (void *) meta;
    posIo->arrayName = arrayName; 
    posIo->array_id = array_id;
    posIo->complete_cb = frontend_io_complete;
    
    /* iterate all reactors, or XXX: for only event reactors? */ 
    //int core = AccelEngineApi::GetReactorByIndex(request_cnt_++ % reactor_cnt_);
    
    /* 
     * XXX: currently, event reactor is not used for foreground io in default
     * only one event reactor is enough, use the first event reactor cpu
     */
    int core = event_reactor_cpu_vec_[0];

    auto eventFrameworkApi = EventFrameworkApiSingleton::Instance();
    eventFrameworkApi->SendSpdkEvent(core, 
            frontend_io_submit, posIo, reinterpret_cast<void *>(core));

    pd_debug("array_id=%d, volume_id=%d, rba=%lu, core=%d\n", 
            array_id, volume_id, rba, core);
}

bool PrefetchDispatcher::FrontendIO(PrefetchMetaSmartPtr meta) {
    int array_id = meta->arrayId;
    int volume_id = meta->volumeId;
    uint64_t rba = rba_start(meta->rba);
    auto read_cache = ReadCacheSingleton::Instance();
    uintptr_t addr;
    int retry_cnt = 0;
    constexpr int max_retry_cnt = 3;
    
    /* already cached? */
    if (read_cache->Contain(array_id, volume_id, 
                ChangeByteToBlock(rba))) {
        return true;
    }
    
    if (!LockRba(array_id, volume_id, rba)) {
        airlog("CNT_ReadCache", "failed_locked_admit", 0, 1);
        return false;
    }
    
    while (retry_cnt++ < max_retry_cnt) {
        addr = read_cache->TryGetBuffer();
        if (addr) {
            break;
        }
        read_cache->Evict();
    }
    if (!addr) {
        UnlockRba(array_id, volume_id, rba, blocks_per_extent);
        return false; /* failed to allocate buffer */
    }
    
    read_cache->Put(array_id, volume_id, ChangeByteToBlock(rba), addr);
    
    size_t remain_size = extent_size;
    
    if (extent_size > max_prefetch_size) {
        size_t size = max_prefetch_size;

        while (1) {
            PrefetchMeta *prefetch_meta = new PrefetchMeta(array_id, volume_id, 
                    rba);
            IssueFrontendIO(prefetch_meta, addr, size);

            remain_size -= size;
            if (remain_size == 0)
                break;

            rba += size;
            addr += size;
            size = remain_size > max_prefetch_size ? max_prefetch_size : 
                remain_size;
        }
    } else {
        // create new meta for pointer
        PrefetchMeta *prefetch_meta = new PrefetchMeta(array_id, volume_id, rba);
        IssueFrontendIO(prefetch_meta, addr, extent_size);
    }

    return true;
}
} // namespace pos
