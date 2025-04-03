
# 源代码文件
obj-m += bdev.o

bdev-objs := main.o

# 添加全局编译选项
EXTRA_CFLAGS += -g -O1

# 编译模块
.PNONY: all 
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

# 清除模块
.PNONY: clean
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
