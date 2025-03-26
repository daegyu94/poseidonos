#include <assert.h>
#include <stdlib.h>     /* malloc, free */
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <numaif.h>

#include <iostream>

#include "extent_pool.h"
#include "stat.h"

using namespace std; 

namespace pos {
ExtentPool::ExtentPool(size_t pool_size, size_t alloc_size) 
    : pool_size_(pool_size), alloc_size_(alloc_size)
{
    void *start_addr;
    bool use_pmem = false;

	if (use_pmem) {
		int fd = open("/mnt/pmem0/file", O_RDWR);
		if (fd < 0) {
			perror("open");
			return;
		}

		start_addr = mmap(nullptr, pool_size, PROT_READ | PROT_WRITE, 
                MAP_SHARED, fd, 0);
        if (start_addr == MAP_FAILED) {
            assert(0);
        }
        printf("[INFO] %s, mmap is succeed, waiting memset...\n", __func__);
    } else {
        bool use_hugepage = true;
        int hugepage_flag = use_hugepage ? MAP_HUGETLB : 0;

        start_addr = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, 
                MAP_PRIVATE | MAP_ANONYMOUS | hugepage_flag, -1, 0);
        if (start_addr == MAP_FAILED) {
            assert(0);
        }

        int numa_node = 0;
        unsigned long nodemask = 1UL << numa_node;
        if (mbind(start_addr, pool_size, MPOL_PREFERRED, &nodemask, 
                    sizeof(nodemask) * 8, 0) == -1) {
            perror("mbind");
            munmap(start_addr, pool_size);
            assert(0);
        }
    }

    memset(start_addr, 0x00, pool_size);

    start_addr_ = (uintptr_t) start_addr;

    total_fl_ = 1;
    num_iter_ = total_fl_ > 1 ? total_fl_ / 2 : 1;
    
    allocs_ = new atomic<size_t>[total_fl_]();
    useds_ = new atomic<size_t>[total_fl_]();
    
    // maximum freelist size
    const int64_t max_chunks = pool_size / alloc_size; 
    const int64_t per_fl_chunks = max_chunks / total_fl_;
    uintptr_t addr = start_addr_;

    per_fl_size_ = per_fl_chunks * alloc_size_;

    freelist_ = new FreeList *[total_fl_];
    for (int i = 0; i < total_fl_; i++) {
        freelist_[i] = new FreeList(addr, max_chunks);

        for (int64_t j = per_fl_chunks - 1; j >= 0; j--) {
            freelist_[i]->PutFreeAddr(addr + ((uintptr_t) j * alloc_size_));
            allocs_[i] += alloc_size_;
        }
        printf("0x%lx\n", addr);
        addr += (per_fl_chunks * alloc_size_);
    }

    printf("[INFO] %s: total_fl=%d, start_addr=0x%lx, pool_size=%lu, alloc_size=%lu\n",
            __func__, total_fl_, start_addr_, pool_size_, alloc_size_);
}

ExtentPool::~ExtentPool() {
    /* TODO: */
}

/* Client-local allocation, 1. current cpu, 2. other cpu */ 
uintptr_t ExtentPool::Allocate() {
    int fl_id = total_fl_ == 1 ? 0 : rand() % total_fl_;
    uintptr_t free_addr = -1; 

    /* try to allocate next freelist*/
    for (int i = 0; i < num_iter_; i++) {
        free_addr = freelist_[fl_id]->GetFreeAddr();
        if (free_addr != (uintptr_t) -1) {
            useds_[fl_id] += alloc_size_;
            readcache_stat.pool_used += alloc_size_;
            break;
        }
        
        /* next fl_id */
        if (fl_id == (total_fl_ - 1))
            fl_id = 0;
        else
            fl_id++;
    }

    return free_addr;
}

void ExtentPool::Free(uintptr_t addr) {
    int fl_id = total_fl_ == 1 ? 0 : rand() % total_fl_;
    
    for (int i = 0; i < num_iter_; i++) {
        if (allocs_[fl_id] < per_fl_size_) {
            break;
        }
        /* next fl_id */
        if (fl_id == (total_fl_ - 1))
            fl_id = 0;
        else
            fl_id++;
    }

    freelist_[fl_id]->PutFreeAddr(addr);

    useds_[fl_id] -= alloc_size_;
    readcache_stat.pool_used -= alloc_size_;
}
}
