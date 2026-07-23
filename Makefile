obj-m += virt_msg-drv.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm -f \
		*.o *.ko *.mod *.mod.c *.mod.o \
		*.symvers modules.order Module.symvers \
		*.cmd \
		.*.o .*.ko .*.mod .*.mod.c .*.mod.o \
		.*.symvers .*.cmd
	rm -rf .tmp_versions .cache
