
# 源代码文件
obj-m += dump.o 

dump-objs := main.o dumps.o

# 添加全局编译选项
EXTRA_CFLAGS += -g -O0

# 编译模块
.PNONY: all 
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc dumps.c test.c -o test

# 清除模块
.PNONY: clean
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -fr test
