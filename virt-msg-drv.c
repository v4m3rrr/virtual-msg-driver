#include <linux/init.h>
#include <linux/device.h>
#include <linux/device/class.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/cdev.h>
#include <linux/slab.h>

/*
 * Defines
 */
#define VMD_LOG_INFO(fmt, ...) \
	pr_info("%s: " fmt "\n", driver_name, ##__VA_ARGS__)

#define VMD_LOG_ERR(fmt, ...) \
	pr_err("%s: " fmt "\n", driver_name, ##__VA_ARGS__)

#define VMD_MAX_DEVICES 8

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

/*
 * Struct definitons
 */
struct msg_device {
	struct device *dev;
	char *buff;
};

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
};

struct msg_device *msg_device = { 0 };

/*
 * Function definitions
 */
static int __init virt_msg_drv_init(void)
{
	int ret;
	int i;

	if (cdev_count <= 0 || cdev_count > VMD_MAX_DEVICES) {
		VMD_LOG_ERR("invalid cdev_count param; VMD_MAX_DEVICES=%d",
			    VMD_MAX_DEVICES);
		ret = -EINVAL;
		goto err_inv_param;
	}

	msg_device = kcalloc(cdev_count, sizeof(struct msg_device), GFP_KERNEL);
	if (!msg_device) {
		VMD_LOG_ERR("failed to allocate memory for message queues");
		ret = -ENOMEM;
		goto err_alloc_msg_dev;
	}

	ret = alloc_chrdev_region(&dev_num, 0, cdev_count, driver_name);
	if (ret) {
		VMD_LOG_ERR("failed alloc_chrdev_region");
		goto err_alloc_reg;
	}

	cls = class_create(cls_name);
	if (IS_ERR(cls)) {
		VMD_LOG_ERR("failed class_create");
		ret = PTR_ERR(cls);
		goto err_class_create;
	}

	for (i = 0; i < cdev_count; ++i) {
		dev_t curr_dev_num = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);

		struct device *p = device_create(cls, NULL, curr_dev_num, NULL,
						 "%s_%d", driver_name, i);
		if (IS_ERR(p)) {
			VMD_LOG_ERR("failed device_create");
			ret = PTR_ERR(p);
			goto err_device_create;
		}
		msg_device[i].dev = p;
	}

	cdev_init(&cdev, &fops);

	ret = cdev_add(&cdev, dev_num, cdev_count);
	if (ret) {
		VMD_LOG_ERR("failed cdev_add");
		goto err_cdev_add;
	}

	VMD_LOG_INFO("init");
	return 0;

err_cdev_add:
err_device_create:
	for (i = i - 1; i >= 0; --i) {
		dev_t curr_dev_num = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
		if (msg_device[i].dev)
			device_destroy(cls, curr_dev_num);
	}
	class_destroy(cls);
err_class_create:
	unregister_chrdev_region(dev_num, cdev_count);
err_alloc_reg:
	kfree(msg_device);
err_alloc_msg_dev:
err_inv_param:
	return ret;
}

static void __exit virt_msg_drv_exit(void)
{
	cdev_del(&cdev);
	for (int i = cdev_count - 1; i >= 0; --i) {
		dev_t curr_dev_num = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
		device_destroy(cls, curr_dev_num);
	}
	class_destroy(cls);
	unregister_chrdev_region(dev_num, cdev_count);
	kfree(msg_device);
	VMD_LOG_INFO("exit");
}

static int virt_msg_open(struct inode *inode, struct file *filp)
{
	dev_t dev_num = inode->i_rdev;
	VMD_LOG_INFO("device(%d:%d) open", MAJOR(dev_num), MINOR(dev_num));
	return 0;
}

static int virt_msg_release(struct inode *inode, struct file *filp)
{
	dev_t dev_num = inode->i_rdev;
	VMD_LOG_INFO("device(%d:%d) release", MAJOR(dev_num), MINOR(dev_num));
	return 0;
}

static ssize_t virt_msg_read(struct file *filp, char __user *buf, size_t count,
			     loff_t *off)
{
	return 0;
}
static ssize_t virt_msg_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *off)
{
	return 0;
}

module_init(virt_msg_drv_init);
module_exit(virt_msg_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("v4m3rrr");
MODULE_DESCRIPTION("Basic virtual message device driver");
