#pragma once 

#include "extent.h"

namespace pos {
class ICachePolicy {
public:
    virtual ~ICachePolicy() = default;
    virtual void Put(const KeyType&, const ValueType &) = 0;
    
    virtual bool Contain(const KeyType&) { return false; }
    virtual void ClearInProgress(const KeyType&, int) { return; }

    virtual int Get(const KeyType&, ValueType &, const RequestExtent &,
            uintptr_t &) = 0;
    virtual int Delete(const KeyType&, ValueType &) = 0;
    virtual void Evict(ValueType &) = 0;
    
    virtual size_t Size(void) { return 0; }
    virtual double Util(void) { return 0.0; }
    virtual void Print(void)  { return; }
};
} // namespace pos
