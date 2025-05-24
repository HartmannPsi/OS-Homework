// drivers/acpi/oemh_table.c
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#define ACPI_SIG_OEMH "OEMH"

struct oemh_info {
  u64 cpu_hz;
  u64 mem_mb;
  u8 gpu_count;
};

static struct oemh_info g_info;

static ssize_t cpu_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%llu\n", g_info.cpu_hz);
}

static ssize_t mem_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%llu\n", g_info.mem_mb);
}

static ssize_t gpu_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%u\n", g_info.gpu_count);
}

static struct kobj_attribute cpu_attr = __ATTR(cpu_hz, 0444, cpu_show, NULL);
static struct kobj_attribute mem_attr = __ATTR(mem_mb, 0444, mem_show, NULL);
static struct kobj_attribute gpu_attr = __ATTR(gpu_count, 0444, gpu_show, NULL);

static struct attribute *attrs[] = {
    &cpu_attr.attr,
    &mem_attr.attr,
    &gpu_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *oemh_kobj;

static int __init oemh_init(void) {
  struct acpi_table_header *table;

  if (ACPI_SUCCESS(acpi_get_table(ACPI_SIG_OEMH, 0, &table))) {
    u8 *data = (u8 *)table + sizeof(struct acpi_table_header);
    g_info.cpu_hz = *(u64 *)(data + 0);
    g_info.mem_mb = *(u64 *)(data + 8);
    g_info.gpu_count = *(u8 *)(data + 16);

    oemh_kobj = kobject_create_and_add("oemh_info", kernel_kobj);
    if (!oemh_kobj) return -ENOMEM;
    return sysfs_create_group(oemh_kobj, &attr_group);
  }

  return -ENODEV;
}

static void __exit oemh_exit(void) {
  if (oemh_kobj) kobject_put(oemh_kobj);
}

module_init(oemh_init);
module_exit(oemh_exit);
