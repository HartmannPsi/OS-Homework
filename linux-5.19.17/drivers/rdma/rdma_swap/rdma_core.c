// rdma_core.c
#include "rdma_core.h"

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <rdma/ib_verbs.h>

#include "rdma_swap.h"

// ================= RDMA 基本结构 =================

struct rdma_context {
  struct ib_device *device;
  struct ib_pd *pd;
  struct ib_cq *cq;
  struct ib_qp *qp;
  struct ib_mr *remote_mr;  // 注册的远程内存
  void *local_buf;          // 本地临时缓冲
  dma_addr_t local_dma;
  u64 remote_addr;
  u32 rkey;
};

static struct rdma_context *ctx;

// ================= 连接初始化（简化模型） =================

static int rdma_setup_context(void) {
  struct ib_device **dev_list;
  int num_devices;

  dev_list = ib_alloc_device_list(&num_devices);
  if (!dev_list || num_devices == 0) return -ENODEV;

  ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
  if (!ctx) return -ENOMEM;

  ctx->device = dev_list[0];
  ib_free_device_list(dev_list);

  ctx->pd = ib_alloc_pd(ctx->device, 0);
  if (IS_ERR(ctx->pd)) return PTR_ERR(ctx->pd);

  ctx->cq = ib_create_cq(ctx->device, NULL, NULL, NULL, 16, 0);
  if (IS_ERR(ctx->cq)) return PTR_ERR(ctx->cq);

  // 创建 QP
  struct ib_qp_init_attr init_attr = {
      .send_cq = ctx->cq,
      .recv_cq = ctx->cq,
      .cap =
          {
              .max_send_wr = 16,
              .max_recv_wr = 16,
              .max_send_sge = 1,
              .max_recv_sge = 1,
          },
      .qp_type = IB_QPT_RC,
  };

  ctx->qp = ib_create_qp(ctx->pd, &init_attr);
  if (IS_ERR(ctx->qp)) return PTR_ERR(ctx->qp);

  // 此处省略 QP 状态转换、连接建立，实际需在用户空间完成 handshake + QP state
  // 管理

  return 0;
}

// ================= RDMA 页面读取 =================

int rdma_core_read_page(void *dst, u64 remote_addr, u32 rkey) {
  struct ib_sge sge;
  struct ib_rdma_wr wr;
  struct ib_send_wr *bad_wr;
  struct ib_wc wc;

  memset(&sge, 0, sizeof(sge));
  sge.addr = (u64)dst;
  sge.length = PAGE_SIZE;
  sge.lkey = ctx->pd->local_dma_lkey;

  memset(&wr, 0, sizeof(wr));
  wr.wr.opcode = IB_WR_RDMA_READ;
  wr.wr.sg_list = &sge;
  wr.wr.num_sge = 1;
  wr.remote_addr = remote_addr;
  wr.rkey = rkey;

  int err = ib_post_send(ctx->qp, &wr.wr, &bad_wr);
  if (err) return err;

  // 等待完成
  while (ib_poll_cq(ctx->cq, 1, &wc) == 0);
  return (wc.status == IB_WC_SUCCESS) ? 0 : -EIO;
}

// ================= 释放资源 =================

void rdma_core_cleanup(void) {
  if (ctx) {
    if (ctx->qp) ib_destroy_qp(ctx->qp);
    if (ctx->cq) ib_destroy_cq(ctx->cq);
    if (ctx->pd) ib_dealloc_pd(ctx->pd);
    kfree(ctx);
  }
}

int rdma_core_init(void) { return rdma_setup_context(); }
