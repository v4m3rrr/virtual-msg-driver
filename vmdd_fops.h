#ifndef VMDD_FOPS_H
#define VMDD_FOPS_H

#include <linux/fs.h>

extern int vmdd_open(struct inode *inode, struct file *filp);
extern int vmdd_release(struct inode *inode, struct file *filp);
extern ssize_t vmdd_read(struct file *filp, char __user *buf, size_t count,
			 loff_t *off);
extern ssize_t vmdd_write(struct file *filp, const char __user *buf,
			  size_t count, loff_t *off);

#endif // VMDD_FOPS_H
