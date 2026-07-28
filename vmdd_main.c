#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>

#include "vmdd.h"
#include "vmdd_device.h"
#include "vmdd_fops.h"
#include "vmdd_ioctl.h"

const char vmdd_module_name[] = "vmdd";
dev_t vmdd_dev_num =
	MKDEV(VMDD_DEFAULT_MAJOR_NUMBER, VMDD_DEFAULT_MINOR_NUMBER);
struct msg_device *vmdd_msg_devices = { 0 };

int vmdd_cdev_count = 3;
module_param(vmdd_cdev_count, int, 0);
MODULE_PARM_DESC(vmdd_cdev_count, "A number of msg queues");

static struct cdev cdev;
static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = vmdd_open,
	.release = vmdd_release,
	.read = vmdd_read,
	.write = vmdd_write,
	.unlocked_ioctl = vmdd_ioctl,
};

static int __init vmdd_init(void)
{
	int ret;

	if (unlikely(vmdd_cdev_count <= 0 ||
		     vmdd_cdev_count > VMDD_MAX_DEVICES)) {
		VMDD_LOG_ERR("invalid cdev_count param; VMD_MAX_DEVICES=%d",
			     VMDD_MAX_DEVICES);
		ret = -EINVAL;
		goto err_inv_param;
	}

	vmdd_msg_devices =
		kcalloc(vmdd_cdev_count, sizeof(*vmdd_msg_devices), GFP_KERNEL);
	if (unlikely(!vmdd_msg_devices)) {
		VMDD_LOG_ERR("failed to allocate memory for message queues");
		ret = -ENOMEM;
		goto err_alloc_msg_dev;
	}

	if (MAJOR(vmdd_dev_num))
		ret = register_chrdev_region(vmdd_dev_num, vmdd_cdev_count,
					     vmdd_module_name);
	else
		ret = alloc_chrdev_region(&vmdd_dev_num, MINOR(vmdd_dev_num),
					  vmdd_cdev_count, vmdd_module_name);

	if (unlikely(ret)) {
		VMDD_LOG_ERR("failed alloc_chrdev_region");
		goto err_alloc_reg;
	}

	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;

	ret = cdev_add(&cdev, vmdd_dev_num, vmdd_cdev_count);
	if (unlikely(ret)) {
		VMDD_LOG_ERR("failed cdev_add");
		goto err_cdev_add;
	}

	ret = vmdd_create_devices();
	if (unlikely(ret)) {
		VMDD_LOG_ERR("failed virt_msg_create_devices");
		goto err_create_devices;
	}

	VMDD_LOG_INFO("init");
	return 0;

err_create_devices:
	cdev_del(&cdev);
err_cdev_add:
	unregister_chrdev_region(vmdd_dev_num, vmdd_cdev_count);
err_alloc_reg:
	kfree(vmdd_msg_devices);
err_alloc_msg_dev:
err_inv_param:
	return ret;
}

static void __exit vmdd_exit(void)
{
	vmdd_destroy_devices();
	cdev_del(&cdev);
	unregister_chrdev_region(vmdd_dev_num, vmdd_cdev_count);
	kfree(vmdd_msg_devices);
	VMDD_LOG_INFO("exit");
}

module_init(vmdd_init);
module_exit(vmdd_exit);

MODULE_LICENSE(VMDD_LICENSE);
MODULE_AUTHOR(VMDD_AUTHOR);
MODULE_DESCRIPTION(VMDD_DESC);
