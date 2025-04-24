#include "calc.h"
#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者
MODULE_DESCRIPTION("A simple kernel module example"); // 模块详细说明信息
MODULE_VERSION("0.01"); // 模块版本

static char *name = "Linux Driver";
module_param(name, charp, S_IRUGO);
static int number = 1;
module_param(number, int, S_IRUGO);

// 模块注册函数，在模块被加载时调用
static int __init hello_init(void)
{
	printk(KERN_INFO "Hello, %s!\n", name);
	printk(KERN_INFO "number is %d\n", number);
	printk(KERN_INFO "a + b = %d\n", add_integer(1, 2));
	return 0;
}

// 模块注销函数，在模块被移除时调用
static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye, world!\n");
}

module_init(hello_init); // 注册模块启动函数
module_exit(hello_exit); // 注册模块注销函数
