// 读写文件扩展属性

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/xattr.h>

int main(int argc, char *argv[]) {
  // 检查参数个数
  if (argc < 4 || argc > 5) {
    printf("Usage: %s <get|set|del> <filename> <attr_name> [value]\n", argv[0]);
    return 1;
  }

  const char *op = argv[1];    // 操作类型
  const char *path = argv[2];  // 操作路径
  const char *name = argv[3];  // 文件名

  if (strcmp(op, "get") == 0 && argc == 4) {
    char value[1024] = {0};
    ssize_t len = getxattr(path, name, value, sizeof(value));
    if (len == -1) {
      perror("getxattr");
      return 1;
    } else
      printf("Value: %s\n", value);

  } else if (strcmp(op, "set") == 0 && argc == 5) {
    if (setxattr(path, name, argv[4], strlen(argv[4]), 0) == -1) {
      perror("setxattr");
      return 1;
    }

  } else if (strcmp(op, "del") == 0 && argc == 4) {
    if (removexattr(path, name) == -1) {
      perror("removexattr");
      return 1;
    }

  } else {
    printf("Invalid usage\n");
    return 1;
  }

  return 0;
}
