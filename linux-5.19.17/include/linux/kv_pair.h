#include <linux/types.h>

struct kv_pair {
  int key;
  int value;
  struct hlist_node node;
}