// persist_mod.c
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "ramfs_persist.h"

#define DEVICE_NAME "ramfs_flush"

static char *sync_dir = NULL;

static int flush_to_dir(struct file *file) {
  struct file *out;
  char *relpath, *abspath;
  loff_t size = i_size_read(file_inode(file));
  char *buf = kzalloc(size, GFP_KERNEL);
  if (!buf) return -ENOMEM;

  kernel_read(file, 0, buf, size);

  relpath = dentry_path_raw(file->f_path.dentry, NULL, 0);
  if (IS_ERR(relpath)) return PTR_ERR(relpath);

  abspath = kasprintf(GFP_KERNEL, "%s/%s", sync_dir, relpath);
  if (!abspath) return -ENOMEM;

  char *tmpfile = kasprintf(GFP_KERNEL, "%s.tmp", abspath);
  out = filp_open(tmpfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (IS_ERR(out)) return PTR_ERR(out);

  kernel_write(out, buf, size, &out->f_pos);
  filp_close(out, NULL);

  vfs_rename_file(tmpfile, abspath);

  kfree(tmpfile);
  kfree(abspath);
  kfree(buf);
  return 0;
}

static long device_ioctl(struct file *filp, unsigned int cmd,
                         unsigned long arg) {
  if (cmd == RAMFS_BIND_IOCTL) {
    char *user_path = kzalloc(256, GFP_KERNEL);
    if (!user_path) return -ENOMEM;
    if (copy_from_user(user_path, (char __user *)arg, 255)) return -EFAULT;
    sync_dir = kstrdup(user_path, GFP_KERNEL);
    pr_info("RAMFS bind path: %s\n", sync_dir);
    kfree(user_path);
    return 0;
  } else if (cmd == RAMFS_FLUSH_IOCTL) {
    struct fd f = fdget(arg);
    if (!f.file) return -EBADF;
    int ret = flush_to_dir(f.file);
    fdput(f);
    return ret;
  }

  return -EINVAL;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = device_ioctl,
};

static struct miscdevice flush_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &fops,
};

static int __init persist_init(void) { return misc_register(&flush_dev); }

static void __exit persist_exit(void) { misc_deregister(&flush_dev); }

module_init(persist_init);
module_exit(persist_exit);
MODULE_LICENSE("GPL");
