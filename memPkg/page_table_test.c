#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MY_IOCTL_GET_PADDR _IOWR('k', 1, struct addr_translation)

// convert virt addr to phys addr
struct addr_translation {
  unsigned long vaddr;
  unsigned long paddr;
};

int main() {
  // open device
  int fd = open("dev/my_device", O_RDWR);
  if (fd < 0) {  // failed to open
    // _THROW("failed to open device");
    perror("failed to open device");
    return 1;
  }

  // map private anonymous memory
  const size_t size = 4096;
  void *va1 = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

  if (va1 == MAP_FAILED) {
    perror("mmap failed at 1");
    return 1;
  }

  // get phys addr1
  struct addr_translation addr = {.vaddr = (unsigned long)va1};
  if (ioctl(fd, MY_IOCTL_GET_PADDR, &addr) < 0) {  // failed to get phys addr
    perror("ioctl failed at 1");
    munmap(va1, size);
    close(fd);
    return 1;
  }
  unsigned long paddr1 = addr.paddr;

  // unmap at 1
  munmap(va1, size);

  // map to the same virt addr
  void *va2 = mmap(va1, size, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (va2 == MAP_FAILED) {
    perror("mmap failed at 2");
    close(fd);
    return 1;
  }

  // get phys addr2
  addr.vaddr = (unsigned long)va2;
  if (ioctl(fd, MY_IOCTL_GET_PADDR, &addr) < 0) {
    perror("ioctl failed at 2");
    munmap(va2, size);
    close(fd);
    return 1;
  }
  unsigned long paddr2 = addr.paddr;

  printf("Physical addr: 0x%lx , 0x%lx", paddr1, paddr2);
  printf("Test &s\n", (paddr1 != paddr2) ? "PASSED" : "FAILED");

  munmap(va2, size);
  close(fd);
  return 0;
}
