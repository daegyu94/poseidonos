#pragma once

#include <iostream>
#include <memory>
#include <string>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "proto/generated/cpp/prefetch.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using prefetch::Prefetcher;
using prefetch::PrefetchReply;
using prefetch::PrefetchRequest;

namespace pos {
class PrefetchWorker;

class PrefetchServiceImpl final {
public:
    PrefetchServiceImpl();
    ~PrefetchServiceImpl();

    void Run(std::string, PrefetchWorker *);
    void SetTerminate(bool flag);

private:
    // Class encompasing the state and logic needed to serve a request.
    class CallData {
    public:
        CallData(Prefetcher::AsyncService* service, ServerCompletionQueue* cq, 
                PrefetchWorker *worker);
        void Proceed();

    private:
        Prefetcher::AsyncService* service_;
        ServerCompletionQueue* cq_;
        ServerContext ctx_;

        PrefetchRequest request_;
        PrefetchReply reply_;

        ServerAsyncResponseWriter<PrefetchReply> responder_;

        PrefetchWorker *worker_;
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_;  // The current serving state.
        
    };

    void HandleRpcs();

    std::unique_ptr<grpc::Server> server_; /* XXX: namespace problem? */
    std::unique_ptr<ServerCompletionQueue> cq_;
    Prefetcher::AsyncService service_;
    PrefetchWorker *worker_;
};
} // namespace pos 
