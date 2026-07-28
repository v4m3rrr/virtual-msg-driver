#include "vmdd_ioctl.h"

#include "vmdd.h"

long vmdd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct msg_device *msg_device;

	msg_device = (struct msg_device *)filp->private_data;

	switch (cmd) {
	case VMDD_IOCTL_GCOUNT:
		int len;
		mutex_lock(&msg_device->mtx);
		len = kfifo_len(&msg_device->queue);
		mutex_unlock(&msg_device->mtx);

		ret = put_user(len, (int __user *)arg);
		break;
	default:
		return -ENOTTY;
	}

	return ret;
}
