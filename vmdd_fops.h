#ifndef VMDD_FOPS_H
#define VMDD_FOPS_H

#include <linux/fs.h>

int vmdd_open(struct inode *inode, struct file *filp);
int vmdd_release(struct inode *inode, struct file *filp);
ssize_t vmdd_read(struct file *filp, char __user *buf, size_t count,
		  loff_t *off);
ssize_t vmdd_write(struct file *filp, const char __user *buf, size_t count,
		   loff_t *off);
__poll_t vmdd_poll(struct file *filp, struct poll_table_struct *wait);
#endif // VMDD_FOPS_H
