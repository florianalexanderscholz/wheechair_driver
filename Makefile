LDLIBS=-lrt -lpthread

ifneq ($(KERNELRELEASE),)
obj-m	:= pcf8591.o

else
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default:
	$(MAKE)	-C $(KDIR)	M=$(PWD) modules
endif

clean:
	rm -rf *.ko *.o *.mod.c 
	rm -rf modules.order Module.symvers
	rm -rf .tmp_versions .*.cmd
