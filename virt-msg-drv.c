#include "asm-generic/errno-base.h"
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/device/class.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/wait.h>

#include "virt_msg.h"

/*
 * Defines
 */
#define VMD_LOG_INFO(fmt, ...) \
	pr_info("%s: " fmt "\n", driver_name, ##__VA_ARGS__)

#define VMD_LOG_ERR(fmt, ...) \
	pr_err("%s: " fmt "\n", driver_name, ##__VA_ARGS__)

#define VMD_MAX_DEVICES 8
#define VMD_QUEUE_ECOUNT 4096

/*
 * Struct definitons
 */
struct msg_device {
	struct device *dev;
	DECLARE_KFIFO_PTR(queue, char);

	struct mutex mtx;
	wait_queue_head_t wait_queue;
};

/*
 * Function declarations
 */
static int __init virt_msg_drv_init(void);
static void __exit virt_msg_drv_exit(void);

static int virt_msg_open(struct inode *, struct file *);
static int virt_msg_release(struct inode *, struct file *);
static ssize_t virt_msg_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t virt_msg_write(struct file *, const char __user *, size_t,
			      loff_t *);

static int virt_msg_create_devices(void);
static void virt_msg_destroy_devices(int);

static ssize_t count_show(struct device *dev, struct device_attribute *attr,
			  char *buf);

static int msg_device_has_data(struct msg_device *msg_device);

static long virt_msg_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg);

/* 
 * Device attributes
 */
static DEVICE_ATTR_RO(count);

/*
 * Global variables
 */
static const char driver_name[] = "virtmsg";
static dev_t dev_num;

static int cdev_count = 3;
module_param(cdev_count, int, 0);
MODULE_PARM_DESC(cdev_count, "A number of msg queues");

static const char *cls_name = driver_name;
static struct class *cls;

static struct cdev cdev;
static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = virt_msg_open,
	.release = virt_msg_release,
	.read = virt_msg_read,
	.write = virt_msg_write,
	.unlocked_ioctl = virt_msg_ioctl,
};

struct msg_device *msg_devices = { 0 };

/*
 * Function definitions
 */
static int __init virt_msg_drv_init(void)
{
	int ret;

	if (cdev_count <= 0 || cdev_count > VMD_MAX_DEVICES) {
		VMD_LOG_ERR("invalid cdev_count param; VMD_MAX_DEVICES=%d",
			    VMD_MAX_DEVICES);
		ret = -EINVAL;
		goto err_inv_param;
	}

	msg_devices = kcalloc(cdev_count, sizeof(*msg_devices), GFP_KERNEL);
	if (!msg_devices) {
		VMD_LOG_ERR("failed to allocate memory for message queues");
		ret = -ENOMEM;
		goto err_alloc_msg_dev;
	}

	ret = alloc_chrdev_region(&dev_num, 0, cdev_count, driver_name);
	if (ret) {
		VMD_LOG_ERR("failed alloc_chrdev_region");
		goto err_alloc_reg;
	}

	cdev_init(&cdev, &fops);

	ret = cdev_add(&cdev, dev_num, cdev_count);
	if (ret) {
		VMD_LOG_ERR("failed cdev_add");
		goto err_cdev_add;
	}

	cls = class_create(cls_name);
	if (IS_ERR(cls)) {
		VMD_LOG_ERR("failed class_create");
		ret = PTR_ERR(cls);
		goto err_class_create;
	}

	ret = virt_msg_create_devices();
	if (ret) {
		VMD_LOG_ERR("failed virt_msg_create_devices");
		goto err_create_devices;
	}

	VMD_LOG_INFO("init");
	return 0;

err_create_devices:
	class_destroy(cls);
err_class_create:
	cdev_del(&cdev);
err_cdev_add:
	unregister_chrdev_region(dev_num, cdev_count);
err_alloc_reg:
	kfree(msg_devices);
err_alloc_msg_dev:
err_inv_param:
	return ret;
}

static void __exit virt_msg_drv_exit(void)
{
	virt_msg_destroy_devices(cdev_count);
	class_destroy(cls);
	cdev_del(&cdev);
	unregister_chrdev_region(dev_num, cdev_count);
	kfree(msg_devices);
	VMD_LOG_INFO("exit");
}

static int virt_msg_open(struct inode *inode, struct file *filp)
{
	int index;

	index = iminor(inode) - MINOR(dev_num);
	filp->private_data = (void *)&msg_devices[index];

	VMD_LOG_INFO("device(%d) open", iminor(inode));
	return 0;
}

static int virt_msg_release(struct inode *inode, struct file *filp)
{
	VMD_LOG_INFO("device(%d) release", iminor(inode));
	return 0;
}

static ssize_t virt_msg_read(struct file *filp, char __user *buf, size_t count,
			     loff_t *off)
{
	struct msg_device *msg_device;
	unsigned copied;
	int ret;
	int is_empty;

	msg_device = (struct msg_device *)filp->private_data;

	is_empty = !msg_device_has_data(msg_device);

	if (is_empty && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;

	ret = wait_event_interruptible(msg_device->wait_queue,
				       msg_device_has_data(msg_device));
	if (ret)
		return ret;

	mutex_lock(&msg_device->mtx);
	ret = kfifo_to_user(&msg_device->queue, buf, count, &copied);
	mutex_unlock(&msg_device->mtx);

	if (ret)
		return ret;

	return copied;
}

static ssize_t virt_msg_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *off)
{
	struct msg_device *msg_device;
	int ret;
	unsigned copied;

	msg_device = (struct msg_device *)filp->private_data;
	mutex_lock(&msg_device->mtx);
	ret = kfifo_from_user(&msg_device->queue, buf, count, &copied);
	mutex_unlock(&msg_device->mtx);

	if (ret)
		return ret;

	if (copied > 0)
		wake_up_interruptible(&msg_device->wait_queue);

	return copied;
}

static int virt_msg_create_devices(void)
{
	int ret;
	int i;

	for (i = 0; i < cdev_count; ++i) {
		dev_t curr_dev_num;
		struct device *p;

		curr_dev_num = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
		p = device_create(cls, NULL, curr_dev_num, NULL, "%s_%d",
				  driver_name, i);
		if (IS_ERR(p)) {
			VMD_LOG_ERR("failed device_create");
			ret = PTR_ERR(p);
			goto failed;
		}
		msg_devices[i].dev = p;

		dev_set_drvdata(p, &msg_devices[i]);
		ret = device_create_file(p, &dev_attr_count);
		if (ret) {
			VMD_LOG_ERR("failed device_create_file");
			device_destroy(cls, curr_dev_num);
			goto failed;
		}

		mutex_init(&msg_devices[i].mtx);

		ret = kfifo_alloc(&msg_devices[i].queue, VMD_QUEUE_ECOUNT,
				  GFP_KERNEL);
		if (ret) {
			VMD_LOG_ERR("failed kfifo_alloc");
			device_remove_file(p, &dev_attr_count);
			device_destroy(cls, curr_dev_num);
			goto failed;
		}

		init_waitqueue_head(&msg_devices[i].wait_queue);
	}

	return 0;
failed:
	virt_msg_destroy_devices(i);
	return ret;
}

static void virt_msg_destroy_devices(int count)
{
	for (int i = 0; i < count; ++i) {
		dev_t curr_dev_num = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);

		kfifo_free(&msg_devices[i].queue);
		mutex_destroy(&msg_devices[i].mtx);
		device_remove_file(msg_devices[i].dev, &dev_attr_count);
		device_destroy(cls, curr_dev_num);
	}
}

static ssize_t count_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct msg_device *msg_device;
	unsigned len;

	msg_device = dev_get_drvdata(dev);

	mutex_lock(&msg_device->mtx);
	len = kfifo_len(&msg_device->queue);
	mutex_unlock(&msg_device->mtx);

	return sysfs_emit(buf, "%u\n", len);
}

static int msg_device_has_data(struct msg_device *msg_device)
{
	int has_data;
	mutex_lock(&msg_device->mtx);
	has_data = !kfifo_is_empty(&msg_device->queue);
	mutex_unlock(&msg_device->mtx);

	return has_data;
}

static long virt_msg_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	int ret;
	struct msg_device *msg_device;

	msg_device = (struct msg_device *)filp->private_data;

	switch (cmd) {
	case VMD_IOCTL_GCOUNT:
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

module_init(virt_msg_drv_init);
module_exit(virt_msg_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("v4m3rrr");
MODULE_DESCRIPTION("Basic virtual message device driver");
