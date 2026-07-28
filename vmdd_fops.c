#include "vmdd_fops.h"

#include "vmdd.h"

#include <linux/kfifo.h>
#include <linux/poll.h>

// I think kfifo operation can be slighlty improved
// using that it is guranteed to be atomic for
// concurrent one reader and writer. Maybe using two mutexes? But then
// it is not guarneteed (I think) that kfifo_{from,to}_user to be atomic.
// Also has_space and has_data can be speed up using spinlocks.

static int queue_has_data(struct msg_device *msg_device);
static int queue_has_space(struct msg_device *msg_device);

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

	msg_device = (struct msg_device *)filp->private_data;

	if (!queue_has_data(msg_device) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	do {
		ret = wait_event_interruptible(msg_device->read_queue,
					       queue_has_data(msg_device));
		if (unlikely(ret))
			return ret;

		mutex_lock(&msg_device->mtx);
		ret = kfifo_to_user(&msg_device->queue, buf, count, &copied);
		mutex_unlock(&msg_device->mtx);

		if (unlikely(ret))
			return ret;
	} while (copied == 0); // Prevents stealing by another process between
	// wait_event (when it returns) and mutex_lock

	if (copied > 0)
		wake_up_interruptible(&msg_device->write_queue);

	return copied;
}

ssize_t vmdd_write(struct file *filp, const char __user *buf, size_t count,
		   loff_t *off)
{
	struct msg_device *msg_device;
	int ret;
	unsigned copied;

	msg_device = (struct msg_device *)filp->private_data;

	if (!queue_has_space(msg_device) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	do {
		ret = wait_event_interruptible(msg_device->write_queue,
					       queue_has_space(msg_device));
		if (unlikely(ret))
			return ret;

		mutex_lock(&msg_device->mtx);
		ret = kfifo_from_user(&msg_device->queue, buf, count, &copied);
		mutex_unlock(&msg_device->mtx);

		if (unlikely(ret))
			return ret;
	} while (copied == 0); // Prevents stealing by another process between
	// wait_event (when it returns) and mutex_lock

	if (copied > 0)
		wake_up_interruptible(&msg_device->read_queue);

	return copied;
}

__poll_t vmdd_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct msg_device *msg_device;
	__poll_t mask;

	msg_device = (struct msg_device *)filp->private_data;
	mask = 0;

	poll_wait(filp, &msg_device->read_queue, wait);
	poll_wait(filp, &msg_device->write_queue, wait);

	if (queue_has_data(msg_device))
		mask |= POLL_IN;
	if (queue_has_space(msg_device))
		mask |= POLL_OUT;

	return mask;
}

static int queue_has_data(struct msg_device *msg_device)
{
	int has_data;
	mutex_lock(&msg_device->mtx);
	has_data = !kfifo_is_empty(&msg_device->queue);
	mutex_unlock(&msg_device->mtx);

	return has_data;
}
static int queue_has_space(struct msg_device *msg_device)
{
	int has_space;

	mutex_lock(&msg_device->mtx);
	has_space = !kfifo_is_full(&msg_device->queue);
	mutex_unlock(&msg_device->mtx);

	return has_space;
}
