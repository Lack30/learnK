#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/module.h>

static struct ftrace_ops ops;
unsigned long target_ip = 0;

static unsigned long lookup_name(const char *name)
{
    struct kprobe kp = { .symbol_name = name };
    unsigned long address = 0;
    int ret = 0;

    ret = register_kprobe(&kp);

    if (ret < 0)
    {
        pr_err("failed registering kprobe for %s", name);
        return 0;
    }
    address = (unsigned long)kp.addr;
    unregister_kprobe(&kp);

    return address;
}

static int (*orig_path_mount)(const char *dev_name, struct path *path,
                              const char *type_page, unsigned long flags,
                              void *data_page);

static int ftrace_path_mount(const char *dev_name, struct path *path,
                             const char *type_page, unsigned long flags,
                             void *data_page)

{
    return orig_path_mount(dev_name, path, type_page, flags, data_page);
}
// 回调函数，在 sys_mount 执行时触发
static void trace_callback(unsigned long ip, unsigned long parent_ip,
                           struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
    struct pt_regs *regs = ftrace_get_regs(fregs);
    regs->ip = parent_ip;
    pr_info("sys_mount called: %lx\n", ip);
}

static int __init __ftrace_init(void)
{
    // 动态获取 sys_mount 的实际符号名（适配不同架构）
    target_ip = lookup_name("path_mount"); // 通用名称（部分内核版本）
    if (!target_ip)
    {
        printk(KERN_ERR "path_mount symbol not found\n");
        return -ENOENT;
    }
    *((unsigned long *)orig_path_mount) = target_ip;

    // 配置 ftrace 操作
    ops.func = trace_callback;
    ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION |
                FTRACE_OPS_FL_IPMODIFY;

    int ret = ftrace_set_filter_ip(&ops, target_ip, 0, 0); // 按地址过滤
    if (ret)
    {
        printk(KERN_ERR "filter setup failed: %d\n", ret);
        unregister_ftrace_function(&ops);
        return ret;
    }

    // 注册 ftrace 并设置过滤器
    ret = register_ftrace_function(&ops);
    if (ret)
    {
        printk(KERN_ERR "ftrace registration failed: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "Monitoring path_mount (IP: 0x%lx)\n", target_ip);
    return 0;
}

static void __exit __ftrace_exit(void)
{
    ftrace_set_filter_ip(&ops, target_ip, 1, 0); // 移除过滤器
    unregister_ftrace_function(&ops);
    printk(KERN_INFO "Module unloaded\n");
}

module_init(__ftrace_init);
module_exit(__ftrace_exit);
MODULE_LICENSE("GPL");