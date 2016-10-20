#Although timers operate on the granularity of jiffies, hrtimers operate at the granularity of nanoseconds

obj-m += hello.o
obj-m += startstop.o
startstop-objs := start.o stop.o world.o

KERNELDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

