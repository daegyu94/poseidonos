#include "src/read_cache/prefetch_worker.h"

#include "src/include/memory.h"
#include "src/read_cache/prefetch_dispatcher.h"

#include <cassert>

namespace pos {
PrefetchWorker::PrefetchWorker() { }

PrefetchWorker::~PrefetchWorker() {
    if (threadPool_)
        delete threadPool_;
    if (dispatcher_)
        delete dispatcher_;
    if (service_)
        delete service_;
}

void PrefetchWorker::Enqueue(PrefetchMetaSmartPtr meta) {
    threadPool_->Enqueue([this, meta] {
                Work(meta);
            }); 
}

void PrefetchWorker::Initialize(int numThreads, std::string svrAddr) {
    threadPool_ = new PrefetchThreadPool(numThreads);
    dispatcher_ = new PrefetchDispatcher();
    service_ = new PrefetchServiceImpl();
    service_->Run(svrAddr, this);
}

bool PrefetchWorker::Work(PrefetchMetaSmartPtr meta) {
    assert(meta != nullptr);
    return dispatcher_->Do(meta);
}
} // namespace pos
