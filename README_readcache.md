# PoseidonOS Write-through Cache

## Principle
- Write operations incurs cache admission and performs NVMe write IO (i.e., write-through cache).
- Since a buffer is allocated with a fixed extent size, a valid block is marked through the bitmap of the extent. 
- We provide simple cache replacement policies (e.g., FIFO, FIFOFastEviction).

## pos.config for cache
- "enable": true: enable, false: disable
- "cache_size_mb": cache memory pool size in MB (please config with 1024 MB unit)
- "cache_policy": 
  - FIFOPolicy: extent is evicted only in list head
  - FIFOFastEvictionPolicy: evict extent which exceeds utilization (x > 70%, x < 30%) while entry in the list
- "extent_size_kb": caching size unit, extent consists of several 4KB blocks, extent size <= 128KB (4KB aligned), extent size > 128KB (128 KB aligned)

example of `pos.conf`
```sh
"read_cache": {
  "enable": true,
  "cache_size_mb": 8192,
  "cache_policy": "FIFOFastEvictionPolicy",
  "extent_size_kb": 32
}
```

## Statistics
You can check the cache hit, miss stats by using ```sudo air_tui```
### Example
```sh
TUI mode: [online], status: [play], page: [81/81],  interval: [3],  timestamp: 2023-8-18:19:45:31, pid: 27938
key {↑/↓}: move, {→/←}: (un)fold, {b}: perv page, {n}: next page, {space}: play/pause
    {1~9}: streaming interval, {i}: init, {o}: run, {x}: stop, {q(esc)}: quit
( )   Top
( )   -Group:BACKEND_TEST
( )   -Group:M9K
( )   -Group:MFS
(*)   -Group:POS_GC
( )   +Group:POS_IO
( )[.]..Node:CNT_AIO_CompleteIOs
( )[.]..Node:LAT_Callback
( )[.]..Node:LAT_WrSb_AllocWriteBuf
( )[.]..Node:PERF_SSD
( )[.]..Node:Q_IOQueue
( )   +Group:POS_RAID
( )[.]..Node:LAT_SegmentRebuildRead
( )[.]..Node:LAT_SegmentRebuildRecover
( )[.]..Node:LAT_SegmentRebuildWrite
( )   +Group:POS_READCACHE
( )[O]++Node:CNT_Prefetcher("count")
( )[O]++Node:CNT_ReadCacheBuffer("count")
      "reactor_20"(27951), index:0x0, filter:"succ_evict" Period(count:0      ), Cumulation(count:318.0K )
      "reactor_20"(27951), index:0x0, filter:"pool" Period(count:88.6M  ), Cumulation(count:-915.8M)
      "reactor_21"(27954), index:0x0, filter:"succ_evict" Period(count:2.1K   ), Cumulation(count:259.6K )
      "reactor_21"(27954), index:0x0, filter:"pool" Period(count:261.9M ), Cumulation(count:2.2G   )
      "reactor_22"(27955), index:0x0, filter:"succ_evict" Period(count:0      ), Cumulation(count:259.0K )
      "reactor_22"(27955), index:0x0, filter:"pool" Period(count:12.5M  ), Cumulation(count:2.9G   )
      "reactor_23"(27956), index:0x0, filter:"succ_evict" Period(count:0      ), Cumulation(count:277.6K )
      "reactor_23"(27956), index:0x0, filter:"pool" Period(count:439.0M ), Cumulation(count:-33.7M )
( )[O]++Node:CNT_ReadCacheRead("count")
      "reactor_20"(27951), index:0x0, filter:"hit_single" Period(count:0      ), Cumulation(count:8      )
      "reactor_20"(27951), index:0x0, filter:"miss_single" Period(count:2.3K   ), Cumulation(count:71.8K  )
      "reactor_20"(27951), index:0x0, filter:"hit_merged" Period(count:0      ), Cumulation(count:238    )
      "reactor_20"(27951), index:0x0, filter:"miss_merged" Period(count:6.7K   ), Cumulation(count:506.0K )
      "reactor_20"(27951), index:0x0, filter:"miss_merged_partial" Period(count:0      ), Cumulation(count:10     )
      "reactor_21"(27954), index:0x0, filter:"hit_single" Period(count:0      ), Cumulation(count:25     )
      "reactor_21"(27954), index:0x0, filter:"miss_single" Period(count:810    ), Cumulation(count:70.4K  )
      "reactor_21"(27954), index:0x0, filter:"hit_merged" Period(count:0      ), Cumulation(count:283    )
      "reactor_21"(27954), index:0x0, filter:"miss_merged" Period(count:1.9K   ), Cumulation(count:502.3K )
      "reactor_21"(27954), index:0x0, filter:"miss_merged_partial" Period(count:0      ), Cumulation(count:27     )
      "reactor_22"(27955), index:0x0, filter:"hit_single" Period(count:0      ), Cumulation(count:4      )
      "reactor_22"(27955), index:0x0, filter:"miss_single" Period(count:1.4K   ), Cumulation(count:74.7K  )
      "reactor_22"(27955), index:0x0, filter:"hit_merged" Period(count:0      ), Cumulation(count:315    )
      "reactor_22"(27955), index:0x0, filter:"miss_merged" Period(count:3.1K   ), Cumulation(count:509.3K )
      "reactor_22"(27955), index:0x0, filter:"miss_merged_partial" Period(count:0      ), Cumulation(count:17     )
      "reactor_23"(27956), index:0x0, filter:"hit_single" Period(count:0      ), Cumulation(count:7      )
      "reactor_23"(27956), index:0x0, filter:"miss_single" Period(count:1.7K   ), Cumulation(count:75.8K  )
      "reactor_23"(27956), index:0x0, filter:"hit_merged" Period(count:0      ), Cumulation(count:392    )
      "reactor_23"(27956), index:0x0, filter:"miss_merged" Period(count:5.1K   ), Cumulation(count:514.1K )
      "reactor_23"(27956), index:0x0, filter:"miss_merged_partial" Period(count:0      ), Cumulation(count:24     )
( )[O]++Node:CNT_ReadCacheWrite("count")
      "reactor_20"(27951), index:0x0, filter:"succ_update" Period(count:21.6K  ), Cumulation(count:2.3M   )
      "reactor_21"(27954), index:0x0, filter:"succ_update" Period(count:81.1K  ), Cumulation(count:2.6M   )
      "reactor_22"(27955), index:0x0, filter:"succ_update" Period(count:3.1K   ), Cumulation(count:2.8M   )
      "reactor_23"(27956), index:0x0, filter:"succ_update" Period(count:107.2K ), Cumulation(count:2.2M   )
```
