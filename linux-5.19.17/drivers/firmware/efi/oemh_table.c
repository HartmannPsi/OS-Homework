// SPDX-License-Identifier: GPL-2.0
#include <linux/acpi.h>
#include <linux/firmware.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#define OEMH_SIG "OEMH"
#define OEMH_SIG_LEN 4

static struct kobject *runtime_info_kobj;
static char runtime_info_buf[128];  // 存放 sysfs 输出内容

// 示例 OEMH 表结构定义（你可以根据实际结构调整）
struct acpi_table_oemh {
  struct acpi_table_header header;
  u32 runtime_value;  // 假设这是你要导出的字段
};

// 从 OEMH 表解析数据，保存到 runtime_info_buf
static int parse_oemh_table(void) {
  struct acpi_table_header *table;
  acpi_status status;

  status = acpi_get_table(OEMH_SIG, 0, &table);
  if (ACPI_FAILURE(status)) {
    pr_err("OEMH: Failed to get ACPI table '%s'\n", OEMH_SIG);
    return -ENODEV;
  }

  struct acpi_table_oemh *oemh = (struct acpi_table_oemh *)table;
  snprintf(runtime_info_buf, sizeof(runtime_info_buf),
           "OEMH runtime value: 0x%08x", oemh->runtime_value);

  pr_info("OEMH: Parsed value: %s\n", runtime_info_buf);
  return 0;
}

// sysfs read 函数
static ssize_t runtime_info_show(struct kobject *kobj,
                                 struct kobj_attribute *attr, char *buf) {
  return scnprintf(buf, PAGE_SIZE, "%s\n", runtime_info_buf);
}

static struct kobj_attribute runtime_info_attr =
    __ATTR(runtime - info, 0444, runtime_info_show, NULL);

// 模块初始化
static int __init oemh_init(void) {
  int ret;

  ret = parse_oemh_table();
  if (ret) return ret;

  // 在 /sys/firmware/uefi/ 下创建 sysfs 节点
  runtime_info_kobj = kobject_create_and_add("uefi", firmware_kobj);
  if (!runtime_info_kobj) return -ENOMEM;

  ret = sysfs_create_file(runtime_info_kobj, &runtime_info_attr.attr);
  if (ret) {
    kobject_put(runtime_info_kobj);
    return ret;
  }

  pr_info("OEMH: /sys/firmware/uefi/runtime-info created\n");
  return 0;
}

// 模块卸载
static void __exit oemh_exit(void) {
  sysfs_remove_file(runtime_info_kobj, &runtime_info_attr.attr);
  kobject_put(runtime_info_kobj);
  pr_info("OEMH: sysfs node removed\n");
}

module_init(oemh_init);
module_exit(oemh_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Psi");
MODULE_DESCRIPTION("OEMH ACPI Table Reader and Sysfs Exporter");
