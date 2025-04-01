
#include "linux/cred.h"
#include <linux/binfmts.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("CyberSecurity");

#ifdef DEBUG
#define DBGINFO(m, ...) pr_debug(KBUILD_MODNAME "-dbg: " m "\n", ##__VA_ARGS__)
#else
#define DBGINFO(m, ...) 
#endif

static struct kretprobe kp = {
	.kp.symbol_name = "do_sys_openat2",
	.data_size = PATH_MAX //kretprobe私有数据,方便在不同函数间传递数据
};

static int handler_pre(struct kretprobe_instance *p,
					   struct pt_regs *regs) //注意,结构和kprobe稍微有点不一样
{
	int slen = 0;
	char *filename = NULL;
	const unsigned char *ps_cwd = NULL;
	const char *__user us_filename =
			(const void *)regs->si; // gcc x64 约定 rsi 为第二个参数
	if (IS_ERR_OR_NULL(us_filename))
		return -EFAULT;

	slen = strnlen_user(
			us_filename,
			PATH_MAX); // strlen 内核版 , 用户空间的东西都需要单独操作,内核不可直接访问用户空间
	if (!slen || slen > PATH_MAX)
		return -EFAULT;

	filename = kzalloc(slen + 1, GFP_ATOMIC);
	if (PTR_ERR_OR_ZERO(filename))
		return -EFAULT;

	if (copy_from_user(filename, us_filename, slen)) {
		return -EFAULT;
	}

	if (strstr(filename, "/run") != NULL ||
		strstr(filename, "/proc") != NULL) //过滤非必要文件
		return 0;

	memcpy(p->data, filename, slen);
	ps_cwd =
			current->mm->owner->fs->pwd.dentry->d_name
					.name; //获取程序执行目录,注意,openat参数为相对目录时需要自己拼接程序执行目录才能得到绝对路径
			
	DBGINFO("OPEN AT %s By PID:%d,CWD:%s", filename, current->mm->owner->pid,
			ps_cwd);
	kfree(filename);
	return 0;
}
extern int close_fd(unsigned fd);
static int handler_post(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	char *filename = ri->data;
	if (IS_ERR_OR_NULL(filename))
		return -EFAULT;

	if (strcmp(filename, "test.txt") == 0) //修改openat返回值 达到保护目的
	{
		unsigned int fd = regs_return_value(regs);
		close_fd(fd);
		regs_set_return_value(regs, -EBADF);
	}
	return 0;
}

static __init int kprobe_init(void)
{
	int ret;
	kp.handler = handler_post;
	kp.entry_handler = handler_pre;
	ret = register_kretprobe(&kp);
	if (ret < 0) {
		printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	printk("Planted return probe at %s: %p\n", kp.kp.symbol_name, kp.kp.addr);
	return 0;
}
static __exit void kprobe_exit(void)
{
	unregister_kretprobe(&kp);
}

module_init(kprobe_init);
module_exit(kprobe_exit);
