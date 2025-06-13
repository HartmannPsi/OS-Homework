#include <cuda_runtime.h>
#include <nccl.h>
#include <stdio.h>
#include <stdlib.h>

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
                                 ncclComm_t comm) {
  cudaStream_t stream;
  float* d_ptr;

  CHECK_CUDA(cudaStreamCreate(&stream));
  CHECK_CUDA(cudaMalloc((void**)&d_ptr, count * sizeof(float)));
  CHECK_CUDA(cudaMemcpyAsync(d_ptr, data, count * sizeof(float),
                             cudaMemcpyHostToDevice, stream));
  CHECK_NCCL(ncclBroadcast(d_ptr, d_ptr, count, ncclFloat, root, comm, stream));
  CHECK_CUDA(cudaStreamSynchronize(stream));
  CHECK_CUDA(cudaFree(d_ptr));

  return ncclSuccess;
}