obj-m += nic.o
obj-m += tcp.o
obj-m += udp.o
obj-m += garp.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)

all:
	@echo $(KERNELDIR)
	@echo $(PWD)
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	make -C $(KERNELDIR) M=$(shell pwd) clean
