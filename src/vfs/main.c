#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include "linux/uaccess.h"

// 模块注册函数，在模块被加载时调用
static int __init __do_init(void)
{
    int ret;
    char *dir_name = "/mnt/dev";
    unsigned int lookup_flags = 1;
    struct path path;
    char __user *dir_name_user = kmalloc(sizeof(char __user *));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
    ret = kern_path(dir_name, lookup_flags, &path);
#else
    copy_to_user(dir_name_user, dir_name, 256);
    ret = user_path_at(AT_FDCWD, , lookup_flags, &path);
#endif //LINUX_VERSION_CODE
    pr_info("dir_name: %s, lookup_flags: %d, ret: %d", dir_name, lookup_flags, ret);
    if (ret)
    {
        pr_err("error finding path: %s", dir_name);
        return -EINVAL;
    }

    return -EINVAL;
}

// 模块注销函数，在模块被移除时调用
static void __exit __do_exit(void) {}

module_init(__do_init); // 注册模块启动函数
module_exit(__do_exit); // 注册模块注销函数

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者