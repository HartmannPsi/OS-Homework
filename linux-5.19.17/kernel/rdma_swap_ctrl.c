// rdma_swap_ctrl.c
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include "rdma_core.h"
#include "rdma_swap.h"

#define MAX_RDMA_PAGES 65536

struct rdma_page_entry {
  unsigned long addr;
  u64 remote_addr;
  u32 rkey;
  bool valid;
};

static struct rdma_page_entry *rdma_page_table;
static int rdma_ratio = 50;  // 默认 50% 使用远程交换
static spinlock_t table_lock;

bool use_rdma_swap(void) { return true; }

void set_rdma_swap_ratio(int percent) { rdma_ratio = clamp(percent, 0, 100); }

int get_rdma_swap_ratio(void) { return rdma_ratio; }

static int find_page_entry(unsigned long addr) {
  int i;
  for (i = 0; i < MAX_RDMA_PAGES; i++) {
    if (rdma_page_table[i].valid && rdma_page_table[i].addr == addr) return i;
  }
  return -1;
}

bool rdma_page_exist(unsigned long addr) { return find_page_entry(addr) >= 0; }

struct page *rdma_swap_try_get_page(unsigned long addr) {
  int idx = find_page_entry(addr);
  if (idx < 0) return NULL;

  struct page *page = alloc_page(GFP_KERNEL);
  if (!page) return NULL;

  void *dst = kmap(page);

  if (rdma_core_read_page(dst, rdma_page_table[idx].remote_addr,
                          rdma_page_table[idx].rkey) != 0) {
    kunmap(page);
    __free_page(page);
    return NULL;
  }

  kunmap(page);
  return page;
}

int rdma_swap_store_page(unsigned long addr, struct page *page) {
  int i;
  spin_lock(&table_lock);
  for (i = 0; i < MAX_RDMA_PAGES; i++) {
    if (!rdma_page_table[i].valid) {
      rdma_page_table[i].addr = addr;
      rdma_page_table[i].remote_addr =
          0xdeadbeef0000 + i * PAGE_SIZE;  // mock remote address
      rdma_page_table[i].rkey = 0x1234;
      rdma_page_table[i].valid = true;
      break;
    }
  }
  spin_unlock(&table_lock);
  return (i < MAX_RDMA_PAGES) ? 0 : -ENOMEM;
}

int rdma_swap_init(void) {
  rdma_page_table = vzalloc(sizeof(struct rdma_page_entry) * MAX_RDMA_PAGES);
  if (!rdma_page_table) return -ENOMEM;

  spin_lock_init(&table_lock);
  return rdma_core_init();
}

void rdma_swap_exit(void) {
  vfree(rdma_page_table);
  rdma_core_cleanup();
}

void rdma_swap_debug_print(void) {
  int i;
  for (i = 0; i < MAX_RDMA_PAGES; i++) {
    if (rdma_page_table[i].valid) {
      printk(KERN_INFO "[RDMA] entry %d: addr=0x%lx remote=0x%llx rkey=%x\n", i,
             rdma_page_table[i].addr, rdma_page_table[i].remote_addr,
             rdma_page_table[i].rkey);
    }
  }
}
