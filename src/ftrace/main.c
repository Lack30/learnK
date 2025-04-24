#include <linux/fs.h>
#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/slab.h>

struct ftrace_hook {
	const char *name;
	void *function;
	void *original;

	unsigned long address;
	struct ftrace_ops ops;
};

#define HOOK(_name, _function, _original)                                                          \
	{                                                                                          \
		.name = (_name), .function = (_function), .original = (_original),                 \
	}

static inline bool dattobd_within_module(unsigned long addr, const struct module *mod)
{
	return within_module(addr, mod);
}

static unsigned long lookup_name(const char *name)
{
	struct kprobe kp = { .symbol_name = name };
	unsigned long address = 0;
	int ret = 0;

	ret = register_kprobe(&kp);

	if (ret < 0) {
		pr_err("failed registering kprobe for %s", name);
		return 0;
	}
	address = (unsigned long)kp.addr;
	unregister_kprobe(&kp);

	return address;
}

static int resolve_hook_address(struct ftrace_hook *hook)
{
	hook->address = lookup_name(hook->name);

	if (!hook->address) {
		pr_err("unresolved symbol: %s", hook->name);
		return -ENOENT;
	}

	pr_info("resolved %s to %lx\n", hook->name, hook->address);
	*((unsigned long *)hook->original) = hook->address;

	return 0;
}

/* asmlinkage long sys_mount(char __user *dev_name, char __user *dir_name,
                char __user *type, unsigned long flags,
				void __user *data);
 */
static int (*orig_path_mount)(const char *dev_name, struct path *path, const char *type_page,
			      unsigned long flags, void *data_page);

static int ftrace_path_mount(const char *dev_name, struct path *path, const char *type_page,
			     unsigned long flags, void *data_page)

{
	int sys_ret = 0;
	char *dir_name = NULL;
	char *buf = NULL;

	buf = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buf) {
		return -ENOMEM;
	}

	dir_name = d_path(path, buf, PATH_MAX);
	pr_info("mount %s to %s\n", dev_name, dir_name);
	sys_ret = orig_path_mount(dev_name, path, type_page, flags, data_page);
	if (sys_ret)
		pr_err("mount %s to %s failed: %d\n", dev_name, dir_name, sys_ret);

	if (buf)
		kfree(buf);
	return sys_ret;
}

// 回调函数，在 hook 执行时触发
static void notrace ftrace_callback_handler(unsigned long ip, unsigned long parent_ip,
					    struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
	struct pt_regs *regs = ftrace_get_regs(fregs);
	struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
	pr_info("ftrace callback: %lx\n", ip);
	if (!dattobd_within_module(parent_ip, THIS_MODULE)) {
		pr_info("set ftrace address %lx\n", (unsigned long)hook->function);
		regs->ip = (unsigned long)hook->function;
	}
}

static struct ftrace_hook path_mount_hook = HOOK("path_mount", ftrace_path_mount, &orig_path_mount);

static int register_hook(struct ftrace_hook *hook)
{
	int ret = 0;

	ret = resolve_hook_address(hook);
	if (ret) {
		pr_err("failed resolving hook address for %s", hook->name);
		return ret;
	}

	hook->ops.func = ftrace_callback_handler;
	// cat /boot/config-$(uname -r) | grep "FTRACE" 查看内核中关于 ftrace 的参数
	//  FTRACE_OPS_FL_SAVE_REGS: 保存寄存器，
	//   需要内核中 CONFIG_DYNAMIC_FTRACE_WITH_REGS=y 或 CONFIG_HAVE_DYNAMIC_FTRACE_WITH_REGS=y
	hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION |
		FTRACE_OPS_FL_IPMODIFY;

	ret = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
	if (ret) {
		pr_err("failed setting ftrace filter ip: %d for %s", ret, hook->name);
		return ret;
	}

	ret = register_ftrace_function(&hook->ops);
	if (ret) {
		pr_err("failed registering ftrace function for %s: %d", hook->name, ret);
		ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
		return ret;
	}

	pr_info("registered ftrace hook for %s", hook->name);

	return ret;
}

static int unregister_hook(struct ftrace_hook *hook)
{
	int ret = 0;

	ret = unregister_ftrace_function(&hook->ops);
	if (ret) {
		pr_err("failed unregistering ftrace function for %s", hook->name);
	}

	ret = ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
	if (ret) {
		pr_err("failed setting ftrace filter ip for %s", hook->name);
	}
	return ret;
}

static int __init __ftrace_init(void)
{
	int ret;
	ret = register_hook(&path_mount_hook); // 注册钩子函数
	if (ret) {
		pr_err("failed to register hook\n");
		return -EINVAL;
	}

	pr_info("Module loaded\n");

	return 0;
}

static void __exit __ftrace_exit(void)
{
	unregister_hook(&path_mount_hook); // 注销钩子函数
	printk(KERN_INFO "Module unloaded\n");
}

module_init(__ftrace_init);
module_exit(__ftrace_exit);
MODULE_LICENSE("GPL");