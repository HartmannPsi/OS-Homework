#ifndef RDMA_CORE_H
#define RDMA_CORE_H

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <rdma/ib_verbs.h>

#define RDMA_IP_STR_LEN 64

struct rdma_context;

int rdma_core_init();
void rdma_core_cleanup();

int rdma_core_read_page(void *dst, u64 remote_addr, u32 rkey);

#endif  // RDMA_CORE_H
