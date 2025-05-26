// persist_user.c
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ramfs_persist.h"

void bind_sync_dir(const char *dir) {
  int fd = open("/dev/ramfs_flush", O_RDWR);
  if (fd < 0) {
    perror("open /dev/ramfs_flush");
    exit(1);
  }
  ioctl(fd, RAMFS_BIND_IOCTL, dir);
  close(fd);
}

void ramfs_file_flush(FILE *fp) {
  int fd = fileno(fp);
  int dev = open("/dev/ramfs_flush", O_RDWR);
  if (dev < 0) {
    perror("open ramfs_flush");
    return;
  }
  ioctl(dev, RAMFS_FLUSH_IOCTL, fd);
  close(dev);
}
