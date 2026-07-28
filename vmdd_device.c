#include "vmdd_device.h"

#include <linux/device.h>

#include "vmdd.h"

static const char *cls_name = VMDD_AUTHOR;
static struct class *cls;

static void destroy_devices(int count);

// Consider later move it to vmdd_device_attr.{h,c} when there will be more
// attributes
static ssize_t count_show(struct device *dev, struct device_attribute *attr,
			  char *buf);
static DEVICE_ATTR_RO(count);

int vmdd_create_devices(void)
{
	int ret;
	int i;

	cls = class_create(cls_name);
	if (unlikely(IS_ERR(cls))) {
		VMDD_LOG_ERR("failed class_create");
		ret = PTR_ERR(cls);
		return ret;
	}

	for (i = 0; i < vmdd_cdev_count; ++i) {
		dev_t curr_dev_num;
		struct device *p;

		curr_dev_num =
			MKDEV(MAJOR(vmdd_dev_num), MINOR(vmdd_dev_num) + i);
		p = device_create(cls, NULL, curr_dev_num, NULL, "%s_%d",
				  vmdd_module_name, i);
		if (unlikely(IS_ERR(p))) {
			VMDD_LOG_ERR("failed device_create");
			ret = PTR_ERR(p);
			goto failed;
		}
		vmdd_msg_devices[i].dev = p;

		dev_set_drvdata(p, &vmdd_msg_devices[i]);
		ret = device_create_file(p, &dev_attr_count);
		if (unlikely(ret)) {
			VMDD_LOG_ERR("failed device_create_file");
			device_destroy(cls, curr_dev_num);
			goto failed;
		}

		mutex_init(&vmdd_msg_devices[i].mtx);

		ret = kfifo_alloc(&vmdd_msg_devices[i].queue, VMDD_QUEUE_ECOUNT,
				  GFP_KERNEL);
		if (unlikely(ret)) {
			VMDD_LOG_ERR("failed kfifo_alloc");
			device_remove_file(p, &dev_attr_count);
			device_destroy(cls, curr_dev_num);
			goto failed;
		}

		init_waitqueue_head(&vmdd_msg_devices[i].wait_queue);
	}

	return 0;
failed:
	destroy_devices(i);
	class_destroy(cls);
	return ret;
}

void vmdd_destroy_devices(void)
{
	destroy_devices(vmdd_cdev_count);
	class_destroy(cls);
}

static void destroy_devices(int count)
{
	for (int i = 0; i < count; ++i) {
		dev_t curr_dev_num =
			MKDEV(MAJOR(vmdd_dev_num), MINOR(vmdd_dev_num) + i);

		kfifo_free(&vmdd_msg_devices[i].queue);
		mutex_destroy(&vmdd_msg_devices[i].mtx);
		device_remove_file(vmdd_msg_devices[i].dev, &dev_attr_count);
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
