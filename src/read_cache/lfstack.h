#pragma once 

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#ifndef __cplusplus
#include <stdatomic.h>
#define ATOMIC_VAR_INIT(value) (value)
#else
#include <atomic>
#define _Atomic(X) std::atomic< X >
#endif

namespace pos {
typedef struct lfstack_node {
    uintptr_t value;
    struct lfstack_node *next;
} lfstack_node;

typedef struct lfstack_head {
    uintptr_t aba;
    struct lfstack_node *node;
} lfstack_head;


class LockFreeStack {
    private:
        struct lfstack_node *node_buffer;
        _Atomic(struct lfstack_head) head, free;
        _Atomic(size_t) size;

    public:
        LockFreeStack(size_t num_elems);
        ~LockFreeStack();

        void push(_Atomic(struct lfstack_head) *, struct lfstack_node *);
        struct lfstack_node *pop(_Atomic(struct lfstack_head) *);

        int stack_push(uintptr_t);
        uintptr_t stack_pop();

    private:
        size_t stack_size() {
            return atomic_load(&size);
        }
};
}
