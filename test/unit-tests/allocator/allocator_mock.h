#include <gmock/gmock.h>

#include <list>
#include <string>
#include <vector>

#include "src/allocator/allocator.h"

namespace pos
{
class MockAllocator : public Allocator
{
public:
    using Allocator::Allocator;
    MOCK_METHOD(int, Init, (), (override));
    MOCK_METHOD(void, Dispose, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(void, Flush, (), (override));
    MOCK_METHOD(bool, VolumeCreated, (VolumeEventBase* volEventBase, VolumeEventPerf* volEventPerf, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(bool, VolumeLoaded, (VolumeEventBase* volEventBase, VolumeEventPerf* volEventPerf, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(bool, VolumeUpdated, (VolumeEventBase* volEventBase, VolumeEventPerf* volEventPerf, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(bool, VolumeMounted, (VolumeEventBase* volEventBase, VolumeEventPerf* volEventPerf, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(void, VolumeDetached, (vector<int> volList, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(bool, VolumeDeleted, (VolumeEventBase* volEventBase, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(bool, VolumeUnmounted, (VolumeEventBase* volEventBase, VolumeArrayInfo* volArrayInfo), (override));
    MOCK_METHOD(void, SetNormalGcThreshold, (uint32_t inputThreshold), (override));
    MOCK_METHOD(void, SetUrgentThreshold, (uint32_t inputThreshold), (override));
    MOCK_METHOD(int, GetMeta, (WBTAllocatorMetaType type, std::string fname, MetaFileIntf* file), (override));
    MOCK_METHOD(int, SetMeta, (WBTAllocatorMetaType type, std::string fname, MetaFileIntf* file), (override));
    MOCK_METHOD(int, GetBitmapLayout, (std::string fname), (override));
    MOCK_METHOD(int, GetInstantMetaInfo, (std::string fname), (override));
    MOCK_METHOD(void, FlushAllUserdataWBT, (), (override));
    MOCK_METHOD(IBlockAllocator*, GetIBlockAllocator, (), (override));
    MOCK_METHOD(IWBStripeAllocator*, GetIWBStripeAllocator, (), (override));
    MOCK_METHOD(IContextManager*, GetIContextManager, (), (override));
    MOCK_METHOD(IContextReplayer*, GetIContextReplayer, (), (override));
};

} // namespace pos
