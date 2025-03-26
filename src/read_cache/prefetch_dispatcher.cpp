#include "src/read_cache/prefetch_dispatcher.h"
#include "src/read_cache/stat.h"

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

//#define PREFETCH_BREAKDOWN
#ifdef PREFETCH_BREAKDOWN
#define prefetch_br_airlog(n, f, i, k) airlog(n, f, i, k)
#else
#define prefetch_br_airlog(n, f, i, k) do {} while (0)
#endif

namespace pos {
std::atomic<int> frontPendingIo[256]; /* XXX: hard-coded max event-cores */
int max_qd = MAX_QD;
int num_event_reactor = 1;

void calculatePrefetchDispatcherConfig(int max_event_reactor) {
    if (MAX_PENDING_IO < MAX_QD) {
        max_qd = MAX_PENDING_IO;
        num_event_reactor = 1;
    } else {
        if (MAX_PENDING_IO % MAX_QD) {
            num_event_reactor = -1;
        }

        max_qd = MAX_QD;
        num_event_reactor = MAX_PENDING_IO / MAX_QD;
    }

    printf("[INFO] %s, MAX_PENDING_IO=%d, MAX_QD=%d "
            "max_event_reactor=%d, num_event_reactor=%d, max_qd=%d\n", 
                __func__, MAX_PENDING_IO, MAX_QD,
                max_event_reactor, num_event_reactor, max_qd);
    if (num_event_reactor <= 0 || num_event_reactor > max_event_reactor) {
        assert(0);
    }
}

PrefetchDispatcher::PrefetchDispatcher(void) : request_cnt_(0) { 
    AffinityManager* affinity_manager = pos::AffinityManagerSingleton::Instance();
    cpu_set_t event_reactor_cpu_set = 
        affinity_manager->GetCpuSet(CoreType::EVENT_REACTOR);
    cpu_set_t frontend_io_reactor_cpu_set = 
        affinity_manager->GetCpuSet(CoreType::REACTOR);

    for (uint32_t cpu = 0; cpu < affinity_manager->GetTotalCore(); cpu++) {
        if (CPU_ISSET(cpu, &event_reactor_cpu_set)) {
            //printf("event_reactor_core=%u\n", cpu);
            event_reactor_cpu_vec_.push_back(cpu);
        } else if (CPU_ISSET(cpu, &frontend_io_reactor_cpu_set)) {
            //printf("frontend_io_reactor_core=%u\n", cpu);
            frontend_io_reactor_cpu_vec_.push_back(cpu);
        }
    }
    event_reactor_cnt_ = event_reactor_cpu_vec_.size();
    calculatePrefetchDispatcherConfig(event_reactor_cnt_);

    frontend_io_reactor_cnt_ = frontend_io_reactor_cpu_vec_.size();
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

static void frontend_io_complete(struct pos_io* posIo, int status) {
    PrefetchMeta *meta = (PrefetchMeta *) posIo->context;
    int array_id = meta->arrayId;
    int volume_id = meta->volumeId;
    uint64_t rba = meta->rba;
    uint64_t start_rba = rba % extent_size ? rba_start(rba) : rba;
 
#ifdef CONFIG_EXTENT_POOL
    //printf("[INFO] %s: addr=0x%lx, addr2=0x%lx, length=%lu, core=%d,%d)\n",
    //        __func__, meta->addr, (uintptr_t) posIo->iov->iov_base, posIo->length,
    //        meta->core, frontPendingIo[meta->core].load());

    memcpy((void *) meta->addr, posIo->iov->iov_base, posIo->length);
    auto read_cache = ReadCacheSingleton::Instance();
    read_cache->ReturnTmpBuffer((uintptr_t) posIo->iov->iov_base);
#endif
   
    prefetch_br_airlog("LAT_PrefetchIdxClear", "begin", volume_id, rba);
    ReadCacheSingleton::Instance()->ClearInProgress(array_id, 
            volume_id, ChangeByteToBlock(start_rba), kPrefetchInProgress);
    prefetch_br_airlog("LAT_PrefetchIdxClear", "end", volume_id, rba);
    
    prefetch_br_airlog("LAT_PrefetchUnLock", "begin", volume_id, rba);
    UnlockRba(array_id, volume_id, rba, posIo->length / BLOCK_SIZE);
    prefetch_br_airlog("LAT_PrefetchUnLock", "end", volume_id, rba);
    
    airlog("LAT_PrefetchIO", "end", volume_id, rba);
    
    airlog("CNT_Prefetcher", "succ_admit", 0, 1); 
    
    pd_debug("array_id=%d, volume_id=%d, rba=%lu, addr=%lu, len=%lu\n", 
            array_id, volume_id, rba, (uintptr_t) posIo->iov->iov_base, 
            posIo->length);

    frontPendingIo[meta->core]--;
    
    delete (PrefetchMeta *) posIo->context;
    delete posIo->iov;
    delete posIo;
}

static void frontend_io_submit(void *arg1, void *arg2) {
    struct pos_io *posIo = static_cast<struct pos_io *>(arg1);
#ifdef PREFETCH_BREAKDOWN
    int volume_id = ((PrefetchMeta *) posIo->context)->volumeId;
    uint64_t rba = posIo->offset;
#endif
    
    prefetch_br_airlog("LAT_PrefetchIssue", "end", volume_id, rba);
    
    prefetch_br_airlog("LAT_PrefetchAsyncIO", "begin", volume_id, rba);
    UNVMfSubmitHandler(posIo);
    prefetch_br_airlog("LAT_PrefetchAsyncIO", "end", volume_id, rba);

    UNVMfCompleteHandler();
}

/* XXX: don't need for data path, use for detect prefetch io */
static char arrayName[4] = "77"; // char arrayName[32] = "POSArray"; 
void PrefetchDispatcher::IssueFrontendIO(PrefetchMeta *meta, uintptr_t addr, 
        size_t size, int core) {
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

    auto eventFrameworkApi = EventFrameworkApiSingleton::Instance();
    eventFrameworkApi->SendSpdkEvent(core, 
            frontend_io_submit, posIo, reinterpret_cast<void *>(core));
    
    pd_debug("array_id=%d, volume_id=%d, rba=%lu, core=%d\n", 
            array_id, volume_id, rba, core);
}

/* TODO: format goto-out */
bool PrefetchDispatcher::FrontendIO(PrefetchMetaSmartPtr meta) {
    int array_id = meta->arrayId;
    int volume_id = meta->volumeId;
    uint64_t rba = rba_start(meta->rba);
    auto read_cache = ReadCacheSingleton::Instance();
    uintptr_t addr;
    //int retry_cnt = 0;
    //constexpr int max_retry_cnt = 5;
    br_declare_ts(all);

    read_cache->UpdateBufferUtil();
    
    if (readcache_stat.write_intensive) {
        if (rand() % 100 < 95) {
            return false;
        }
    }

    pd_debug("rba=%lu, start_rba=%lu, blk_addr=%lu\n", 
            meta->rba, rba, ChangeByteToBlock(rba));

    /* already cached? */
    br_start_ts(all);
    if (read_cache->Contain(array_id, volume_id, 
                ChangeByteToBlock(rba))) {
        airlog("CNT_Prefetcher", "failed_admit_contained", 0, 1);
        return true;
    }
    br_end_ts(all, BR_SEG_CACHE);
     
    airlog("LAT_PrefetchIO", "begin", volume_id, rba);

    prefetch_br_airlog("LAT_PrefetchLock", "begin", volume_id, rba);
    br_start_ts(all);
    if (!LockRba(array_id, volume_id, rba)) {
        airlog("CNT_Prefetcher", "failed_admit_locked", 0, 1);
        return false;
    }
    br_end_ts(all, BR_RBA_LOCK);
    prefetch_br_airlog("LAT_PrefetchLock", "end", volume_id, rba);
    
    /* find idle core */
    int core;
#if 1
    while (true) {
        core = event_reactor_cpu_vec_[request_cnt_++ % num_event_reactor];
        if (frontPendingIo[core] < max_qd) {
            break;
        }
        usleep(1);
    }
#else 
    core = event_reactor_cpu_vec_[request_cnt_++ % num_event_reactor];
    if (frontPendingIo[core] >= max_qd) {
        UnlockRba(array_id, volume_id, rba, extent_size / BLOCK_SIZE);
        return false;
    }
#endif
    frontPendingIo[core]++;

    prefetch_br_airlog("LAT_PrefetchBuffer", "begin", volume_id, rba);
    br_start_ts(all);
    //while (retry_cnt++ < max_retry_cnt) 
    while (true)
    {
#ifdef CONFIG_EXTENT_POOL
        addr = read_cache->TryGetTmpBuffer();
        if (addr) {
            break;
        }
#else
        addr = read_cache->TryGetBuffer();
        if (addr) {
            break;
        }
        read_cache->Evict();
#endif
        usleep(1);
    }
    /*
    if (!addr) {
        airlog("CNT_Prefetcher", "failed_admit_unalloc", 0, 1);
        UnlockRba(array_id, volume_id, rba, blocks_per_extent);
        return false;
    }
    */
    br_end_ts(all, BR_SEG_ALLOC);
    prefetch_br_airlog("LAT_PrefetchBuffer", "end", volume_id, rba);

#ifdef CONFIG_EXTENT_POOL
    uintptr_t addr2;
    while (1) {
        addr2 = read_cache->TryGetBuffer();
        if (addr2) {
            break;
        }
        read_cache->Evict();
        usleep(1);
    }
#endif

    prefetch_br_airlog("LAT_PrefetchIdxPut", "begin", volume_id, rba);
    br_start_ts(all);
#ifdef CONFIG_EXTENT_POOL
    read_cache->Put(array_id, volume_id, ChangeByteToBlock(rba), addr2);
#else
    read_cache->Put(array_id, volume_id, ChangeByteToBlock(rba), addr);
#endif
    br_end_ts(all, BR_SEG_CACHE);
    prefetch_br_airlog("LAT_PrefetchIdxPut", "end", volume_id, rba);
    
    br_start_ts(all);
    size_t remain_size = extent_size;
    if (extent_size > max_prefetch_size) {
        size_t size = max_prefetch_size;

        while (1) {
            prefetch_br_airlog("LAT_PrefetchIssue", "begin", volume_id, rba);
#ifdef CONFIG_EXTENT_POOL
            PrefetchMeta *prefetch_meta = new PrefetchMeta(array_id, volume_id, 
                    rba, core, addr2);
#else
            PrefetchMeta *prefetch_meta = new PrefetchMeta(array_id, volume_id, 
                    rba, core);
#endif
            IssueFrontendIO(prefetch_meta, addr, size, core);

            remain_size -= size;
            if (remain_size == 0)
                break;

            rba += size;
            addr += size;
            size = remain_size > max_prefetch_size ? max_prefetch_size : 
                remain_size;
        }
    } else {
        if (1) {
            prefetch_br_airlog("LAT_PrefetchIssue", "begin", volume_id, rba);
            // create new meta for pointer
#ifdef CONFIG_EXTENT_POOL
            PrefetchMeta *prefetch_meta = new PrefetchMeta(array_id, volume_id, 
                    rba, core, addr2);
#else
            PrefetchMeta *prefetch_meta = new PrefetchMeta(array_id, volume_id, core, rba);
#endif
            IssueFrontendIO(prefetch_meta, addr, extent_size, core);
        } else {
            /* XXX: to figure out IO issue overhead */
            ReadCacheSingleton::Instance()->ClearInProgress(array_id, 
                    volume_id, ChangeByteToBlock(rba), kPrefetchInProgress);
            UnlockRba(array_id, volume_id, rba, extent_size / BLOCK_SIZE);
        }
    }
    br_end_ts(all, BR_ISSUE_IO);
    readcache_stat_inc(RC_ISSUED_IO, 1);

    return true;
}
} // namespace pos
