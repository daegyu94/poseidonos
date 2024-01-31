#!/bin/bash

hugepage_mem_size_gb=$1
if [ $# -ne 1 ]; then
  echo "$0 <hugepage_mem_size_gb: 0 means clear>"
  exit -1
fi

# change working directory to where script exists
cd $(dirname $0)

echo "Setting up Hugepages..."

echo 3 > /proc/sys/vm/drop_caches
cd ../lib/spdk/scripts;

nvme_devs=$(lsblk | grep nvme)
if [ ! -z  $nvme_devs ]; then
  echo "[INFO] PCIe to SPDK"
  sudo ./setup.sh reset
else
  echo "[INFO] Already setup with SPDK"
fi

DEF_HUGEPAGESIZE_KB=$(cat /proc/meminfo | grep Hugepagesize | awk '{print $2}')
hugepage_mem_size=$((hugepage_mem_size_gb * 1024 * 1024 * 1024))
def_hugepage_size_kb=$(cat /proc/meminfo | grep Hugepagesize | awk '{print $2}')
num_hugepages=$((hugepage_mem_size / 1024 / def_hugepage_size_kb))

echo "[INFO] hugepage_mem_size=$hugepage_mem_size, def_hugepage_size_kb=$def_hugepage_size_kb, num_hugepages=$num_hugepages"

sudo HUGE_EVEN_ALLOC=yes NRHUGE=${num_hugepages} ./setup.sh; 

numastat -m | grep -E "HugePages_|Hugepage"

cd -;

#SETUP_CORE_DUMP
ulimit -c unlimited
systemctl disable apport.service
mkdir -p /etc/pos/core
echo "/etc/pos/core/%E.core" > /proc/sys/kernel/core_pattern


#SETUP_MAX_MAP_COUNT
MAX_MAP_COUNT=65535
CURRENT_MAX_MAP_COUNT=$(cat /proc/sys/vm/max_map_count)
echo "Current maximum # of memory map areas per process is $CURRENT_MAX_MAP_COUNT."
if [ "$CURRENT_MAX_MAP_COUNT" -lt "$MAX_MAP_COUNT" ]; then
    echo "Setting maximum # of memory map areas per process to $MAX_MAP_COUNT."
    sudo sysctl -w vm.max_map_count=${MAX_MAP_COUNT}
fi

modprobe nvme-tcp

echo "Setup env. done!"

rm -rf /dev/shm/ibof_nvmf_trace*
