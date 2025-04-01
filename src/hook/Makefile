
obj-m += hook.o

hook-objs := main.o

EXTRA_CFLAGS += -g -O1

.PNONY: all 
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

.PNONY: clean
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
