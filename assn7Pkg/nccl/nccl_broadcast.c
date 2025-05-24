#include <cuda_runtime.h>
#include <nccl.h>
#include <stdio.h>
#include <stdlib.h>

#include "nccl_alt.h"

// 错误检查宏
#define CHECK_CUDA(cmd)                                     \
  do {                                                      \
    cudaError_t e = cmd;                                    \
    if (e != cudaSuccess) {                                 \
      printf("CUDA error %s:%d '%s'\n", __FILE__, __LINE__, \
             cudaGetErrorString(e));                        \
      exit(EXIT_FAILURE);                                   \
    }                                                       \
  } while (0)

#define CHECK_NCCL(cmd)                                     \
  do {                                                      \
    ncclResult_t r = cmd;                                   \
    if (r != ncclSuccess) {                                 \
      printf("NCCL error %s:%d '%s'\n", __FILE__, __LINE__, \
             ncclGetErrorString(r));                        \
      exit(EXIT_FAILURE);                                   \
    }                                                       \
  } while (0)

// 广播操作
ncclResult_t nccl_broadcast_data(void* data, size_t count, int root,
                                 ncclComm_t comm, cudaStream_t stream) {
  return ncclBroadcast(data, data, count, ncclFloat, root, comm, stream);
}

// AllReduce 操作
ncclResult_t nccl_allreduce_data(void* sendbuf, void* recvbuf, size_t count,
                                 ncclComm_t comm, cudaStream_t stream) {
  return ncclAllReduce(sendbuf, recvbuf, count, ncclFloat, ncclSum, comm,
                       stream);
}

// 初始化设备并执行测试
void run_nccl_demo(int num_devices) {
  float* send_buffers[8];
  float* recv_buffers[8];
  int devices[8];

  ncclComm_t comms[8];
  cudaStream_t streams[8];

  for (int i = 0; i < num_devices; i++) {
    devices[i] = i;
    CHECK_CUDA(cudaSetDevice(i));
    CHECK_CUDA(cudaMalloc(&send_buffers[i], sizeof(float) * 16));
    CHECK_CUDA(cudaMalloc(&recv_buffers[i], sizeof(float) * 16));
    CHECK_CUDA(cudaStreamCreate(&streams[i]));

    // 初始化数据：仅第一个 GPU 写入非零数据用于广播
    if (i == 0) {
      float temp[16];
      for (int j = 0; j < 16; j++) temp[j] = j * 1.0f;
      CHECK_CUDA(cudaMemcpy(send_buffers[i], temp, sizeof(float) * 16,
                            cudaMemcpyHostToDevice));
    }
  }

  // 初始化 NCCL 通信器
  CHECK_NCCL(ncclCommInitAll(comms, num_devices, devices));

  // 1. 广播操作
  for (int i = 0; i < num_devices; i++) {
    CHECK_CUDA(cudaSetDevice(i));
    CHECK_NCCL(
        nccl_broadcast_data(send_buffers[i], 16, 0, comms[i], streams[i]));
  }

  // 等待所有设备完成广播
  for (int i = 0; i < num_devices; i++) {
    CHECK_CUDA(cudaSetDevice(i));
    CHECK_CUDA(cudaStreamSynchronize(streams[i]));
  }

  // 2. AllReduce 操作（所有设备加和）
  for (int i = 0; i < num_devices; i++) {
    CHECK_CUDA(cudaSetDevice(i));
    CHECK_NCCL(nccl_allreduce_data(send_buffers[i], recv_buffers[i], 16,
                                   comms[i], streams[i]));
  }

  // 同步
  for (int i = 0; i < num_devices; i++) {
    CHECK_CUDA(cudaStreamSynchronize(streams[i]));
  }

  // 打印结果（在 GPU0）
  float result[16];
  CHECK_CUDA(cudaSetDevice(0));
  CHECK_CUDA(cudaMemcpy(result, recv_buffers[0], sizeof(float) * 16,
                        cudaMemcpyDeviceToHost));
  printf("AllReduce result:\n");
  for (int i = 0; i < 16; i++) {
    printf("%.1f ", result[i]);
  }
  printf("\n");

  // 清理资源
  for (int i = 0; i < num_devices; i++) {
    ncclCommDestroy(comms[i]);
    cudaFree(send_buffers[i]);
    cudaFree(recv_buffers[i]);
    cudaStreamDestroy(streams[i]);
  }
}
