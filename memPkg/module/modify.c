#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

// convert virt addr to phys addr
struct addr_translation {
  unsigned long vaddr;  // virtual addr
  unsigned long paddr;  // physical addr
};

static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  struct addr_translation addr;

  if (copy_from_user(&addr, (void __user *)arg, sizeof(addr))) return -1;

  unsigned long vaddr = addr.vaddr;
  pgd_t *pgd;
  p4d_t *p4d;
  pud_t *pud;
  pmd_t *pmd;
  pte_t *pte;
  struct page *page;
  unsigned long paddr;

  struct mm_struct *mm = current->mm;
  spin_lock(&mm->page_table_lock);

  pgd = pgd_offset(mm, vaddr);
  if (pgd_none(*pgd) || pgd_bad(*pgd)) goto unlock;

  p4d = p4d_offset(pgd, vaddr);
  if (p4d_none(*p4d) || p4d_bad(*p4d)) goto unlock;

  pud = pud_offset(p4d, vaddr);
  if (pud_none(*pud) || pud_bad(*pud)) goto unlock;

  pmd = pmd_offset(pud, vaddr);
  if (pmd_none(*pmd) || pmd_bad(*pmd)) goto unlock;

  pte = pte_offset_kernel(pmd, vaddr);
  if (!pte || !pte_present(*pte)) goto unlock;

  page = pte_page(*pte);
  paddr = page_to_phys(page) | (vaddr & ~PAGE_MASK);
  spin_unlock(&mm->page_table_lock);

  addr.paddr = paddr;
  if (copy_to_user((void __user *)arg, &addr, sizeof(addr))) return -1;

  return 0;

unlock:
  spin_unlock(&mm->page_table_lock);
  return -1;
}

static struct file_operations my_fops = {
    .unlocked_ioctl = my_ioctl,
    .owner = THIS_MODULE,
};

static struct miscdevice my_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "my_device",
    .fops = &my_fops,
};

static int __init my_init(void) { return misc_register(&my_miscdev); }

static void __exit my_exit(void) { misc_deregister(&my_miscdev); }

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");