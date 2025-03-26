#pragma once

#include <climits>
#include <cstring>

#include "freelist.h"

namespace pos {
class ExtentPool {
    private:
        int total_fl_;
        uintptr_t start_addr_;
        size_t pool_size_;
		size_t alloc_size_;
    
        int num_iter_;
        size_t per_fl_size_;

        std::atomic<size_t> *allocs_;
        std::atomic<size_t> *useds_;
    
        FreeList **freelist_;

    public:
        ExtentPool(size_t, size_t);
        ~ExtentPool();

        uintptr_t Allocate();
        void Free(uintptr_t);

        uintptr_t GetStartAddr() const {
            return start_addr_;
        }
        
        size_t GetAlloc(int fl_id) const {
            return allocs_[fl_id].load();
        }

        size_t GetUsed(int fl_id) const {
            return useds_[fl_id].load();
        }

        size_t GetUsed() {
            size_t used_mem = 0;
            for (int i = 0; i < total_fl_; i++)
                used_mem += useds_[i].load();
            return used_mem;
        }

        size_t GetFree(int fl_id) {
            return allocs_[fl_id] - GetUsed(fl_id);
        }

        size_t GetFree() {
            return pool_size_ - GetUsed();
        }

        double GetUtil() {
            return 100.0 * GetUsed() / pool_size_;
        }
        
        void Show() {
            printf("[INFO] util(%%)=%.2f, total=%lu, used=%lu, free=%lu\n", 
                    GetUtil(), pool_size_, GetUsed(), GetFree());
        }
    private:
        ExtentPool(ExtentPool &allocator);
};
}
