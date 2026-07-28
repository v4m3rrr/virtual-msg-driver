#ifndef VMDD_H
#define VMDD_H

#include <linux/kfifo.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/printk.h>

struct msg_device {
	struct device *dev;
	DECLARE_KFIFO_PTR(queue, char);

	struct mutex mtx;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
};

extern const char vmdd_module_name[];
extern dev_t vmdd_dev_num;
extern int vmdd_cdev_count;
extern struct msg_device *vmdd_msg_devices;

#define VMDD_AUTHOR "v4m3rrr"
#define VMDD_LICENSE "GPL"
#define VMDD_DESC "Basic virtual message device driver"

#define VMDD_DEFAULT_MAJOR_NUMBER 0
#define VMDD_DEFAULT_MINOR_NUMBER 0

#define VMDD_LOG_INFO(fmt, ...) \
	pr_info("%s: " fmt "\n", vmdd_module_name, ##__VA_ARGS__)

#define VMDD_LOG_ERR(fmt, ...) \
	pr_err("%s: " fmt "\n", vmdd_module_name, ##__VA_ARGS__)

#define VMDD_MAX_DEVICES 8
#define VMDD_QUEUE_ECOUNT 32

#endif // VMDD_H
