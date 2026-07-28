#ifndef VMDD_IOCTL_H
#define VMDD_IOCTL_H

#include <linux/ioctl.h>
#include <linux/fs.h>

#define VMDD_IOCTL_MAGIC 'V'
#define VMDD_IOCTL_GCOUNT _IOR(VMDD_IOCTL_MAGIC, 0, int)
#define VMDD_IOCTL_SDEVICES _IOW(VMDD_IOCTL_MAGIC, 1, int)

long vmdd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#endif // VMDD_IOCTL_H
