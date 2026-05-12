obj-m += poulsbo_lab.o

KVER ?= 4.15.0-213-generic
KDIR ?= /lib/modules/$(KVER)/build
PWD := $(CURDIR)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
