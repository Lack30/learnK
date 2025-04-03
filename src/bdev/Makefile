
# 源代码文件
obj-m += bdev.o

bdev-objs := main.o

KDR=/lib/modules/$(shell uname -r)/build
# KDR=/root/project/c/linux-3.10.108/build

# 添加全局编译选项
EXTRA_CFLAGS += -g -O1 -DDEBUG

# 编译模块
.PNONY: all 
all:
	make -C $(KDR) M=$(PWD) modules

# 清除模块
.PNONY: clean
clean:
	make -C $(KDR) M=$(PWD) clean
