#ifndef _LINUX_RDMA_SWAP_H
#define _LINUX_RDMA_SWAP_H

#include <linux/mm.h>
#include <linux/types.h>

/*
 * RDMA Swap 控制接口头文件
 * 定义对远程内存 Swap 的读写操作接口
 * 用于在内核各子模块之间共享接口
 */

// 初始化 RDMA 连接（启动时调用）
int rdma_swap_init(void);

// 释放 RDMA 资源（关闭时调用）
void rdma_swap_exit(void);

// 尝试通过 RDMA 从远程读取页面，失败则返回 NULL
struct page *rdma_swap_try_get_page(unsigned long addr);

// 将页面写入远程交换区，成功返回 0，失败返回负数
int rdma_swap_store_page(unsigned long addr, struct page *page);

// 判断是否启用 RDMA swap 功能
bool use_rdma_swap(void);

// 设置 RDMA / local swap 的比例（0 - 100）
void set_rdma_swap_ratio(int percent);
int get_rdma_swap_ratio(void);

// 远程页是否存在（模拟或从页表查询）
bool rdma_page_exist(unsigned long addr);

// 用于 debug 输出（可选）
void rdma_swap_debug_print(void);

#endif /* _LINUX_RDMA_SWAP_H */
