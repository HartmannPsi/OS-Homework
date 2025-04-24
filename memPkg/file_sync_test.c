#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void test_file(const char *filename, size_t size) {
  // open file
  int fd = open(filename, O_RDWR | O_CREAT, 0666);
  if (fd < 0) {
    perror("failed to open file");
    return;
  }

  // justify the file size
  if (ftruncate(fd, size) == -1) {
    perror("failed to justify file size at 1");
    close(fd);
    return;
  }

  // get file state
  struct stat st;
  fstat(fd, &st);

  // map file to mem
  void *addr =
      mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    perror("failed to map file to mem");
    close(fd);
    return;
  }

  // test1 : write after read
  memset(addr, 0xaa, st.st_size);    // fill data 0xaa
  msync(addr, st.st_size, MS_SYNC);  // sync to the disk

  // test2 write through pages
  if (st.st_size >= 4096) {
    char *cross =
        (char *)addr + 4096 - 2;  // locate to the last 2 bytes of page 1
    memset(cross, 0xbb, 4);       // write 4 bytes
    msync(cross, 4, MS_SYNC);
  }

  // test3: write additional
  size_t new_size = st.st_size + 4096;
  if (ftruncate(fd, new_size) == -1) {
    perror("failed to justify file size at 2");
    munmap(addr, st.st_size);
    close(fd);
    return;
  }
  // remap the extended file allowing to move the mapped addr
  void *new_addr = mremap(addr, st.st_size, new_size, MREMAP_MAYMOVE);
  if (new_addr == MAP_FAILED) {
    perror("failed to remap file");
    munmap(addr, st.st_size);
    return;
  }
  memset((char *)new_addr + st.st_size, 0xcc,
         4096);  // fill data at extended area
  msync(new_addr, new_size, MS_SYNC);

  munmap(new_addr, new_size);
  close(fd);
  printf("TEST PASSED: %s", filename);
  return;
}

int main() {
  test_file("empty_file", 0);
  test_file("4k_file", 4096);
  test_file("1m_file", 1024 * 1024);
  return 0;
}