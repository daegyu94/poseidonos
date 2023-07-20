#include "src/read_cache/grpc_prefetch_server.h"
#include "src/read_cache/prefetch_worker.h"
#include "src/read_cache/read_cache.h"

namespace pos {
PrefetchServiceImpl::PrefetchServiceImpl() { }

PrefetchServiceImpl::~PrefetchServiceImpl() {
    server_->Shutdown();
    cq_->Shutdown();
}

void PrefetchServiceImpl::Run(std::string svr_addr, PrefetchWorker *worker) {
    std::string server_address(svr_addr);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    worker_ = worker;
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Grpc Prefetch server listening on " << 
        server_address << std::endl;

#if 0
    // Proceed to the server's main loop.
    HandleRpcs();
#else
    std::thread bgThread(&PrefetchServiceImpl::HandleRpcs, this);
    bgThread.detach();
#endif
}

static std::atomic<bool> need_terminate{false};
void PrefetchServiceImpl::SetTerminate(bool flag) {
    need_terminate.store(flag);
}

void PrefetchServiceImpl::HandleRpcs() {
    new CallData(&service_, cq_.get(), worker_); /* XXX: not delete okay ?! */
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) { 
        GPR_ASSERT(cq_->Next(&tag, &ok));
        if (need_terminate.load()) {
            printf("%s is terminated...\n", __func__);
            break;
        }
        GPR_ASSERT(ok);
        static_cast<CallData*>(tag)->Proceed();
    }
}


// Class encompasing the state and logic needed to serve a request.
PrefetchServiceImpl::CallData::CallData(Prefetcher::AsyncService* service, 
        ServerCompletionQueue* cq, PrefetchWorker *worker)
        : service_(service), cq_(cq), responder_(&ctx_), 
        worker_(worker), status_(CREATE) {
            Proceed();
        }

void PrefetchServiceImpl::CallData::Proceed() {
    if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestPrefetchData(&ctx_, &request_, &responder_, 
                cq_, cq_, this);
    } else if (status_ == PROCESS) {
        new CallData(service_, cq_, worker_);
        
        auto read_cache = ReadCacheSingleton::Instance();
        if (read_cache->IsEnabledPrefetch()) {
            for (const auto& msg : request_.msgs()) {
                auto meta = std::make_shared<PrefetchMeta>(
                        msg.subsys_id() - 1, 
                        msg.ns_id() - 1, 
                        msg.pba()
                        );
                worker_->Enqueue(meta);
            }
        }
         
        //reply_.set_success(true);

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    } else {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}
} // namespace pos 
