obj-m += vmdd.o
vmdd-y := vmdd_main.o vmdd_fops.o vmdd_device.o vmdd_ioctl.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm -f \
		*.o *.ko *.mod *.mod.c *.mod.o \
		*.symvers modules.order Module.symvers \
		*.cmd \
		.*.o .*.ko .*.mod .*.mod.c .*.mod.o \
		.*.symvers .*.cmd .swp
	rm -rf .tmp_versions .cache
