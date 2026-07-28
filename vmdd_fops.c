#include "vmdd_fops.h"

#include "vmdd.h"

static int queue_has_data(struct msg_device *msg_device);

int vmdd_open(struct inode *inode, struct file *filp)
{
	int index;

	index = iminor(inode) - MINOR(vmdd_dev_num);
	filp->private_data = (void *)&vmdd_msg_devices[index];

	VMDD_LOG_INFO("device(%d) open", iminor(inode));
	return 0;
}

int vmdd_release(struct inode *inode, struct file *filp)
{
	VMDD_LOG_INFO("device(%d) release", iminor(inode));
	return 0;
}

ssize_t vmdd_read(struct file *filp, char __user *buf, size_t count,
		  loff_t *off)
{
	struct msg_device *msg_device;
	unsigned copied;
	int ret;
	int is_empty;

	msg_device = (struct msg_device *)filp->private_data;

	is_empty = !queue_has_data(msg_device);

	if (is_empty && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	ret = wait_event_interruptible(msg_device->wait_queue,
				       queue_has_data(msg_device));
	if (unlikely(ret))
		return ret;

	mutex_lock(&msg_device->mtx);
	ret = kfifo_to_user(&msg_device->queue, buf, count, &copied);
	mutex_unlock(&msg_device->mtx);

	if (unlikely(ret))
		return ret;

	return copied;
}

ssize_t vmdd_write(struct file *filp, const char __user *buf, size_t count,
		   loff_t *off)
{
	struct msg_device *msg_device;
	int ret;
	unsigned copied;

	msg_device = (struct msg_device *)filp->private_data;
	mutex_lock(&msg_device->mtx);
	ret = kfifo_from_user(&msg_device->queue, buf, count, &copied);
	mutex_unlock(&msg_device->mtx);

	if (unlikely(ret))
		return ret;

	if (copied > 0)
		wake_up_interruptible(&msg_device->wait_queue);

	return copied;
}

static int queue_has_data(struct msg_device *msg_device)
{
	int has_data;
	mutex_lock(&msg_device->mtx);
	has_data = !kfifo_is_empty(&msg_device->queue);
	mutex_unlock(&msg_device->mtx);

	return has_data;
}
