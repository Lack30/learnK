#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>

// 模块注册函数，在模块被加载时调用
static int __init __do_init(void)
{
    pr_info("shift = %ld\n", PAGE_SHIFT);
    pr_info("%ld/%d=%d\n", PAGE_SIZE, SECTOR_SIZE, PAGE_SIZE / SECTOR_SIZE);
    return -EINVAL;
}

// 模块注销函数，在模块被移除时调用
static void __exit __do_exit(void) {}

module_init(__do_init); // 注册模块启动函数
module_exit(__do_exit); // 注册模块注销函数

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者