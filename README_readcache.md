# PoseidonOS ReadCache

### Prerequisite
- Prefetching is triggered by receiving gRPC messages from initiator
- Please refer to `src/read_cache/grpc_prefetch_server.cpp` and `proto/prefetch.proto`

### Limitation
- Prefetcher, which consumes received prefetch message, is pinned at core 29 and it uses event reactor's core 3, please set pos.config "event_reactor": "3"
  - TODO: change hardcoded config

- Prefetch metadata message consists of subsystem id, namespace id, and physical block address. (e.g., (1, 1, 4096))
  - subsystem nqn should be like "nqn.2019-04.pos:subsystem1" ("subsystem name" + "1"(starts from 1))
  - namespace id is the last single digit of "/dev/nvme0n1"
  - physical block address is address of each volume (file's page offset should be translated by using fiemap() ioctl)
  ```cpp
    auto meta = std::make_shared<PrefetchMeta>(
         msg.subsys_id() - 1,
         msg.ns_id() - 1,
         msg.pba()
         );
  ```
  
### Description of pos.config for read cache
- "enable": true: enable, false: disable
- "cache_size_mb": cache memory pool size in MB
- "cache_policy": FIFOPolicy: extent is evicted only in list head, FIFOFastEvictionPolicy: evict fully used extent while extent in the list
- "extent_size_kb": caching size unit, extent consists of several 4KB blocks, extent size <= 128KB (4KB aligned), extent size > 128KB (128 KB aligned)

example of `pos.conf`
```sh
"read_cache": {
  "enable": true,
  "cache_size_mb": 8192,
  "cache_policy": "FIFOFastEvictionPolicy",
  "extent_size_kb": 128
}
```
