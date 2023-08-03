/*
 *   BSD LICENSE
 *   Copyright (c) 2021 Samsung Electronics Corporation
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Samsung Electronics Corporation nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "src/io/frontend_io/read_submission.h"

#include <air/Air.h>
#include <unistd.h>

#include "spdk/event.h"
#include "src/admin/smart_log_mgr.h"
#include "src/array/array.h"
#include "src/array/device/array_device.h"
#include "src/bio/volume_io.h"
#include "src/device/base/ublock_device.h"
#include "src/dump/dump_module.h"
#include "src/dump/dump_module.hpp"
#include "src/event_scheduler/callback.h"
#include "src/include/pos_event_id.hpp"
#include "src/read_cache/read_cache.h"

//#define READCACHE_READ_BREAKDOWN
#ifdef READCACHE_READ_BREAKDOWN
#define read_br_airlog(n, f, i, k) airlog(n, f, i, k)
#else
#define read_br_airlog(n, f, i, k) do {} while (0)
#endif

namespace pos
{
ReadSubmission::ReadSubmission(VolumeIoSmartPtr volumeIo, BlockAlignment* blockAlignment_, Merger* merger_, Translator* translator_)
: Event(true),
  blockAlignment(blockAlignment_),
  merger(merger_),
  translator(translator_),
  volumeIo(volumeIo)
{
    if (nullptr == blockAlignment)
    {
        blockAlignment = new BlockAlignment{ChangeSectorToByte(volumeIo->GetSectorRba()), volumeIo->GetSize()};
    }
    if (nullptr == merger)
    {
        merger = new Merger{volumeIo, &readCompletionFactory};
    }
    if (nullptr == translator)
    {
        translator = new Translator{volumeIo->GetVolumeId(), blockAlignment->GetHeadBlock(), blockAlignment->GetBlockCount(), volumeIo->GetArrayId(), true};
    }
    airlog("RequestedUserRead", "user", GetEventType(), 1);
}

ReadSubmission::~ReadSubmission()
{
    if (nullptr != blockAlignment)
    {
        delete blockAlignment;
        blockAlignment = nullptr;
    }
    if (nullptr != merger)
    {
        delete merger;
        merger = nullptr;
    }
    if (nullptr != translator)
    {
        delete translator;
        translator = nullptr;
    }
}

bool ReadSubmission::_IsSingleBlockCached(void) {
    auto read_cache = ReadCacheSingleton::Instance();
    bool ret = 0;

    if (read_cache->IsEnabled() && read_cache->IsEnabledCheckCache() &&
            !volumeIo->IsPrefetchIo()) {
        uintptr_t addr = 0;
        BlkAddr blk_addr = blockAlignment->GetHeadBlock();
        int array_id = volumeIo->GetArrayId();
        uint32_t volume_id = volumeIo->GetVolumeId();
        std::pair<BlkAddr, bool> blk_addr_p = std::make_pair(blk_addr, false);

        read_br_airlog("LAT_SingleBlockRead", "begin", volume_id, blk_addr);
        
        ret = read_cache->Get(array_id, volume_id, blk_addr_p, addr);

        if (ret) {
            void *src = (void *) (addr + blockAlignment->GetHeadPosition());
            void *dst = volumeIo->GetBuffer();
            size_t size = volumeIo->GetSize();

            memcpy(dst, src, size);
            
            if (blk_addr_p.second) {
                uintptr_t addr2 = 0;
                read_cache->Delete(array_id, volume_id, blk_addr, addr2);
            } else {
                read_cache->ClearInProgress(array_id, volume_id, blk_addr, 
                        kMemcpyInProgress);
            }

            airlog("CNT_ReadCacheRead", "hit_single", volume_id, 1);
            volumeIo->GetCallback()->Execute(); /* trigger aio completion */
            volumeIo = nullptr;
        } else {
            airlog("CNT_ReadCacheRead", "miss_single", volume_id, 1);
        }
        read_br_airlog("LAT_SingleBlockRead", "end", volume_id, blk_addr);
        
        airlog("HIST_ReadCache", "read_blk_cnt", volume_id, 1);
    }

    return ret;
}

bool
ReadSubmission::Execute(void)
{
    uint32_t volId = volumeIo->GetVolumeId();
    uint32_t arrayId = volumeIo->GetArrayId();
    SmartLogMgrSingleton::Instance()->IncreaseReadCmds(volId, arrayId);
    SmartLogMgrSingleton::Instance()->IncreaseReadBytes(blockAlignment->GetBlockCount(), volId, arrayId);
    bool isInSingleBlock = (blockAlignment->GetBlockCount() == 1);
    if (isInSingleBlock)
    {
        if (_IsSingleBlockCached())
            return true;
        
        _PrepareSingleBlock();
        _SendVolumeIo(volumeIo);
    }
    else
    {
        _PrepareMergedIo();
        _ProcessMergedIo();
    }

    volumeIo = nullptr;
    return true;
}

void
ReadSubmission::_PrepareSingleBlock(void)
{
    PhysicalBlkAddr pba = translator->GetPba();
    pba.lba = blockAlignment->AlignHeadLba(0, pba.lba);
    volumeIo->SetPba(pba);
    StripeAddr lsidEntry;
    bool referenced;
    std::tie(lsidEntry, referenced) = translator->GetLsidRefResult(0);
    if (referenced)
    {
        CallbackSmartPtr callee(volumeIo->GetCallback());
        CallbackSmartPtr readCompletion(new ReadCompletion(volumeIo));
        readCompletion->SetCallee(callee);
        volumeIo->SetCallback(readCompletion);
        volumeIo->SetLsidEntry(lsidEntry);
        callee->SetWaitingCount(1);
    }
}

void
ReadSubmission::_PrepareMergedIo(void)
{
    uint32_t blockCount = blockAlignment->GetBlockCount();

    for (uint32_t blockIndex = 0; blockIndex < blockCount; blockIndex++)
    {
        _MergeBlock(blockIndex);
    }

    merger->Cut();
}

void
ReadSubmission::_MergeBlock(uint32_t blockIndex)
{
    PhysicalBlkAddr pba = translator->GetPba(blockIndex);
    VirtualBlkAddr vsa = translator->GetVsa(blockIndex);
    StripeAddr lsidEntry = translator->GetLsidEntry(blockIndex);
    uint32_t dataSize = blockAlignment->GetDataSize(blockIndex);
    pba.lba = blockAlignment->AlignHeadLba(blockIndex, pba.lba);
    merger->Add(pba, vsa, lsidEntry, dataSize);
}

bool ReadSubmission::_IsMergedBlockCached(uint32_t volumeIoIndex) {
    auto read_cache = ReadCacheSingleton::Instance();
    bool cached = false;

    if (read_cache->IsEnabled() && read_cache->IsEnabledCheckCache() && 
            !volumeIo->IsPrefetchIo()) {
        VolumeIoSmartPtr spVolumeIo = merger->GetSplit(volumeIoIndex);
        BlockAlignment blkAlignment(
                ChangeSectorToByte(spVolumeIo->GetSectorRba()), 
                spVolumeIo->GetSize());
        BlkAddr blk_addr = blkAlignment.GetHeadBlock();
        uint32_t blockCount = blkAlignment.GetBlockCount();
        std::vector<std::pair<uintptr_t, bool>> addrs(blockCount);
        std::vector<std::pair<BlkAddr, bool>> blk_addr_p_vec;
        int array_id = volumeIo->GetArrayId();
        uint32_t volume_id = volumeIo->GetVolumeId();
        
        read_br_airlog("LAT_MergedBlocksRead", "begin", volume_id, blk_addr);
        
        uint32_t num_found = read_cache->Scan(array_id, volume_id, blk_addr, 
                blockCount, addrs, blk_addr_p_vec, true);
        
        assert(blockCount >= num_found);

        /* TODO: split again when only some blocks are cached */
        if (num_found == blockCount) {
            uintptr_t buffer_addr = (uintptr_t) spVolumeIo->GetBuffer();

            for (uint32_t i = 0; i < blockCount; i++) {
                size_t size = blkAlignment.GetDataSize(i); 
                void *src = (i == 0) ? 
                    (void *) (addrs[i].first + blkAlignment.GetHeadPosition()) :
                    (void *) (addrs[i].first);
                void *dst = (void *) buffer_addr;

                //printf("(%u, %d), src=%lu, dst=%lu, size=%lu, " 
                //      "buffer_size=%lu, %u\n", 
                //        blockCount, i, (uintptr_t) src, (uintptr_t) dst, size, 
                //        spVolumeIo->GetSize(), blkAlignment.GetHeadPosition());
                
                memcpy(dst, src, size);
                
                buffer_addr += size;
            }
            
            for (auto iter = blk_addr_p_vec.begin(); 
                    iter != blk_addr_p_vec.end(); 
                    iter++) {
                BlkAddr blk_addr = iter->first;
                bool is_inv = iter->second;
                
                if (is_inv) {
                    uintptr_t addr = 0;
                    read_cache->Delete(array_id, volume_id, blk_addr, addr);
                } else {
                    read_cache->ClearInProgress(array_id, volume_id, blk_addr, 
                            kMemcpyInProgress);
                }
            }
            
            airlog("CNT_ReadCacheRead", "hit_merged", volume_id, blockCount);
            /* ReadCompletion will destroy spVolumeIo */
            spVolumeIo->GetCallback()->Execute();

            cached = true;
        } else {
            /* hit partial */
            if (num_found) {
                for (auto iter = blk_addr_p_vec.begin(); 
                        iter != blk_addr_p_vec.end(); 
                        iter++) {
                    BlkAddr blk_addr = iter->first;
                    bool is_inv = iter->second;
                    
                    if (is_inv) {
                        uintptr_t addr = 0;
                        read_cache->Delete(array_id, volume_id, blk_addr, addr);
                    } else {
                        read_cache->ClearInProgress(array_id, volume_id, 
                                blk_addr, kMemcpyInProgress);
                    }
                }
                airlog("CNT_ReadCacheRead", "miss_merged_partial", volume_id, 
                        blockCount - num_found);
            }
            airlog("CNT_ReadCacheRead", "miss_merged", volume_id, blockCount);
        }

        read_br_airlog("LAT_MergedBlocksRead", "end", volume_id, blk_addr);
        
        airlog("HIST_ReadCache", "read_blk_cnt", volume_id, blockCount);
    }

    return cached;
}

void
ReadSubmission::_ProcessMergedIo(void)
{
    uint32_t volumeIoCount = merger->GetSplitCount();
    CallbackSmartPtr callback = volumeIo->GetCallback();
    callback->SetWaitingCount(volumeIoCount);

    for (uint32_t volumeIoIndex = 0; volumeIoIndex < volumeIoCount;
         volumeIoIndex++)
    {
        if (_IsMergedBlockCached(volumeIoIndex)) {
            continue;
        }
        
        _ProcessVolumeIo(volumeIoIndex);
    }
}

void
ReadSubmission::_ProcessVolumeIo(uint32_t volumeIoIndex)
{
    VolumeIoSmartPtr volumeIo = merger->GetSplit(volumeIoIndex);
    _SendVolumeIo(volumeIo);
}

} // namespace pos
