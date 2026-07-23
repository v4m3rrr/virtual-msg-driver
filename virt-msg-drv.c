#include <linux/module.h>
#include <linux/printk.h>

static char driver_name[] = "virt-msg-drv";

static int __init virt_msg_drv_init(void)
{
	pr_info("%s: Init\n", driver_name);
	return 0;
}

static void __exit virt_msg_drv_exit(void)
{
	pr_info("%s: Exit\n", driver_name);
}

module_init(virt_msg_drv_init);
module_exit(virt_msg_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("v4m3rrr");
MODULE_DESCRIPTION("Basic virtual message device driver");
