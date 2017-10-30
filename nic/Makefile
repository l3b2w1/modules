
obj-m += nic.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)

all:
	@echo $(KERNELDIR)
	@echo $(PWD)
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	make -C $(KERNELDIR) M=$(shell pwd) clean
