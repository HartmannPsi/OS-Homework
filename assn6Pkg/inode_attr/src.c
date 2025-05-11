#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/xattr.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s <get|set|del> <filename> <attr_name> [value]\n", argv[0]);
    return 1;
  }

  const char *op = argv[1];
  const char *file = argv[2];

  if (strcmp(op, "get") == 0 && argc == 4) {
    char value[1024] = {0};
    ssize_t len = getxattr(file, argv[3], value, sizeof(value));
    if (len == -1)
      perror("getxattr");
    else
      printf("Value: %s\n", value);
  } else if (strcmp(op, "set") == 0 && argc == 5) {
    if (setxattr(file, argv[3], argv[4], strlen(argv[4]), 0) == -1)
      perror("setxattr");
  } else if (strcmp(op, "del") == 0 && argc == 4) {
    if (removexattr(file, argv[3]) == -1) perror("removexattr");
  } else {
    printf("Invalid usage\n");
  }

  return 0;
}
