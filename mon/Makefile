
obj-m += mon.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)

all:
	@echo $(KERNELDIR)
	@echo $(PWD)
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

