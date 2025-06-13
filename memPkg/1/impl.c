#include "impl.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief 重新映射一块虚拟内存区域
 * @param addr 原始映射的内存地址，如果为 NULL 则由系统自动选择一个合适的地址
 * @param size 需要映射的大小（单位：字节）
 * @return 成功返回映射的地址，失败返回 NULL
 * @details 该函数用于重新映射一个新的虚拟内存区域。如果 addr 参数为 NULL，
 *          系统会自动选择一个合适的地址进行映射。映射的内存区域大小为 size
 * 字节。 映射失败时返回 NULL。
 */
void* mmap_remap(void* addr, size_t size) {
  // 如果原始地址为空，不需要 remap，直接返回 NULL
  if (addr == NULL || size == 0) {
    perror("mmap NULL or size = 0");
    return NULL;
  }

  void* new_addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (new_addr == MAP_FAILED) {
    perror("mmap failed in remap");
    return NULL;
  }

  memcpy(new_addr, addr, size);

  return new_addr;
}

/**
 * @brief 使用 mmap 进行文件读写
 * @param filename 待操作的文件路径
 * @param offset 写入文件的偏移量（单位：字节）
 * @param content 要写入文件的内容
 * @return 成功返回 0，失败返回 -1
 * @details 该函数使用内存映射（mmap）的方式进行文件写入操作。
 *          通过 filename 指定要写入的文件，
 *          offset 指定写入的起始位置，
 *          content 指定要写入的内容。
 *          写入成功返回 0，失败返回 -1。
 */
int file_mmap_write(const char* filename, size_t offset, char* content) {
  if (!filename || !content) {
    return -1;
  }

  size_t content_len = strlen(content);
  size_t write_end = offset + content_len;

  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    perror("open");
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    perror("fstat");
    close(fd);
    return -1;
  }

  size_t file_size = st.st_size;

  if (write_end > file_size) {
    if (ftruncate(fd, write_end) < 0) {
      perror("ftruncate");
      close(fd);
      return -1;
    }
  }

  void* map = mmap(NULL, write_end, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (map == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return -1;
  }

  memcpy((char*)map + offset, content, content_len);

  if (msync(map, write_end, MS_SYNC) < 0) {
    perror("msync");
    munmap(map, write_end);
    close(fd);
    return -1;
  }

  munmap(map, write_end);
  close(fd);

  return 0;
}