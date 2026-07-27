#ifndef VIRT_MSG_H
#define VIRT_MSG_H

#include <linux/ioctl.h>

#define VMD_IOCTL_MAGIC 'V'
#define VMD_IOCTL_GCOUNT _IOR(VMD_IOCTL_MAGIC, 0, int)
#define VMD_IOCTL_SDEVICES _IOW(VMD_IOCTL_MAGIC, 1, int)

#endif // VIRT_MSG_H
