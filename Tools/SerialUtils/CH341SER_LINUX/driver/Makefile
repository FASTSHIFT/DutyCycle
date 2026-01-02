CONFIG_MODULE_SIG=n

ifeq ($(KERNELRELEASE), )
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD :=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD)  
clean:
	rm -rf *.mk .tmp_versions Module.symvers *.mod.c *.o *.ko .*.cmd Module.markers modules.order *.a *.mod
load:
	insmod ch341.ko
unload:
	rmmod ch341
install: default
	rmmod ch341 || true
	# Remove existing driver to prevent conflicts (e.g. compressed .ko.xz)
	find /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/ -name "ch341.ko*" -delete
	mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/
	cp -f ./ch341.ko /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/
	depmod -a
	modprobe ch341

uninstall:
	rmmod ch341 || true
	find /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/ -name "ch341.ko*" -delete
	depmod -a
else
	obj-m := ch341.o
endif
