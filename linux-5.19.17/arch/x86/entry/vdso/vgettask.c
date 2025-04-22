#include <linux/compiler.h>
#include <linux/sched.h>
#include <linux/vgettask.h>

notrace int __vdso_get_task_info(struct task_info __user *info) {
  struct task_info tmp = {0};

  // 通过 x86 的 gs 寄存器获取当前 task_struct
  struct task_struct *current;
  asm("mov %%gs:0, %0" : "=r"(current));

  // 填充用户态可见数据（避免暴露内核指针）
  tmp.pid = task_pid_nr(current);
  tmp.task_struct_ptr = (void __user *)current;

  // 将数据复制到用户态缓冲区
  if (copy_to_user(info, &tmp, sizeof(tmp))) return -1;

  return 0;
}