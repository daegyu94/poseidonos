#pragma once

#include "src/lib/singleton.h"
#include "src/read_cache/prefetch_dispatcher.h"
#include "src/read_cache/grpc_prefetch_server.h"

#include <atomic>
#include <vector>
#include <thread>
#include <functional>
#include <tbb/concurrent_queue.h>

namespace pos {
/* numa binding? */
class PrefetchThreadPool {
public:
    PrefetchThreadPool(int numThreads) : stop_(false) {
        for (int i = 0; i < numThreads; ++i) {
            threads_.emplace_back([this] {
                std::function<void()> task;
                while (!stop_) {
                    if (tasks_.try_pop(task)) {
                        task();
                    } else {
                        /* XXX: how long should sleep? */
                        std::this_thread::sleep_for(
                                std::chrono::microseconds(5));
                    }
                }
            });
            
            // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
            // only CPU i as set.
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(29, &cpuset); // XXX
            int rc = pthread_setaffinity_np(threads_[i].native_handle(),
                    sizeof(cpu_set_t), &cpuset);
            if (rc != 0) {
                std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
            } else {
                printf("%s: tid=%d is pinnned at CPU 29\n", __func__, i); 
            }
        }
    }

    ~PrefetchThreadPool() {
        stop_ = true;
        for (std::thread& thread : threads_)
            thread.join();
    }

    template <class F>
    void Enqueue(F&& f) {
        tasks_.push(std::forward<F>(f));
    }

private:
    std::vector<std::thread> threads_;
    tbb::concurrent_queue<std::function<void()>> tasks_;
    std::atomic<bool> stop_;
};

class PrefetchWorker {
public:
    PrefetchWorker(void);
    ~PrefetchWorker(void);

    void Initialize(int, std::string);
    void SetTerminate(bool flag) {
        service_->SetTerminate(flag);
    }
    void Enqueue(PrefetchMetaSmartPtr);

private:
    bool Work(PrefetchMetaSmartPtr);

    /* TODO : create a thread pool and in charge of each initiator */
    //std::thread *thread_;
    PrefetchThreadPool *threadPool_;
    PrefetchDispatcher *dispatcher_;
    PrefetchServiceImpl *service_;
};
using PrefetchWorkerSingleton = Singleton<PrefetchWorker>;
} // namespace pos
