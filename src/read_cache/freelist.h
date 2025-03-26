#pragma once

#include "lfstack.h"

namespace pos {
class FreeList {
    private:
        uintptr_t start_addr;

        LockFreeStack *freelist;

    public:
        FreeList(uintptr_t start_addr, int64_t max_chunks) {
            this->start_addr = start_addr;
            // Create a linked-list with all free positions
            freelist = new LockFreeStack(max_chunks);
        }

        ~FreeList() {
            delete freelist;
        }

        uintptr_t GetFreeAddr() {
            return freelist->stack_pop();
        }

        void PutFreeAddr(uintptr_t addr) {
            freelist->stack_push(addr);
        }
};
}
