#include "impl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

void get_inode_info(const char *path) {
  struct stat st;
  if (stat(path, &st) == -1) {
    perror("stat");
    return;
  }

  printf("Inode information for: %s\n", path);
  printf("Inode number: %lu\n", st.st_ino);
  printf("Device ID: %lu\n", st.st_dev);
  printf("File mode: %o\n", st.st_mode);
  printf("Number of hard links: %lu\n", st.st_nlink);
  printf("UID: %u\n", st.st_uid);
  printf("GID: %u\n", st.st_gid);
  printf("File size: %ld bytes\n", st.st_size);
  printf("Last accessed: %ld\n", st.st_atime);
  printf("Last modified: %ld\n", st.st_mtime);
  printf("Last status change: %ld\n", st.st_ctime);
}

char *get_xattr(const char *path, const char *name) {
  char buf[1024] = {0};
  ssize_t ret = getxattr(path, name, buf, sizeof(buf));

  if (ret < 0) {
    printf("getxattr failed: %ld\n", ret);
    return NULL;
  }

  char *result = (char *)malloc(ret + 1);
  if (!result) return NULL;
  strcpy(result, buf);
  result[ret] = '\0';
  return result;
}

int set_xattr(const char *path, const char *name, const char *value) {
  return setxattr(path, name, value, strlen(value), 0) == 0 ? 1 : -1;
}

int remove_xattr(const char *path, const char *name) {
  return removexattr(path, name) == 0 ? 1 : -1;
}

void list_xattrs(const char *path) {
  ssize_t len = listxattr(path, NULL, 0);
  if (len == -1) {
    perror("listxattr");
    return;
  }

  if (len == 0) {
    printf("No extended attributes.\n");
    return;
  }

  char *list = malloc(len);
  if (!list) {
    perror("malloc");
    return;
  }

  len = listxattr(path, list, len);
  if (len == -1) {
    perror("listxattr");
    free(list);
    return;
  }

  for (char *p = list; p < list + len;) {
    printf("xattr: %s\n", p);
    p += strlen(p) + 1;
  }

  free(list);
}
