#include <linux/compiler_types.h>
#include <linux/types.h>

// vDSO暴露接口
struct task_info {
  pid_t pid;
  void __user *task_struct_ptr;
  // 其他关键字段...
};

extern int __vdso_get_task_info(struct task_info *info) __attribute__((weak));

static inline int get_task_struct_info(struct task_info *info) {
  if (__vdso_get_task_info) return __vdso_get_task_info(info);
  return -1;
}