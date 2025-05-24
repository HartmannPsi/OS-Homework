#ifndef NCCL_ALT_H
#define NCCL_ALT_H

#include <cuda_runtime.h>
#include <nccl.h>
#include <stddef.h>

ncclResult_t nccl_broadcast_data(void* data, size_t count, int root,
                                 ncclComm_t comm, cudaStream_t stream);

ncclResult_t nccl_allreduce_data(void* sendbuf, void* recvbuf, size_t count,
                                 ncclComm_t comm, cudaStream_t stream);

void run_nccl_demo(int num_devices);

#endif
