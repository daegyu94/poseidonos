#pragma once

#include <tbb/concurrent_queue.h>

#include "src/read_cache/prefetch_meta.h"

namespace pos {
/* 'T' should be a pointer type of certain object */
template<typename T>
class PrefetchMetaQueue
{
public:
    PrefetchMetaQueue(void) { }

    virtual ~PrefetchMetaQueue(void) { }

    virtual bool IsEmpty(void) const {
        return queue_.empty();
    }

    virtual void Enqueue(const T obj) {
        queue_.push(obj);
    }

    virtual T Dequeue(void) {
        T t;
        if (queue_.try_pop(t))
            return t;
        else
            return nullptr;
    }

private:
    tbb::concurrent_queue<T> queue_;
};
} // namespace pos

