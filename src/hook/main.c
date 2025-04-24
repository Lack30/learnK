#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define KPS_SIZE 10

static struct kprobe kps[KPS_SIZE];

static void build_path(struct dentry *dentry, char *buffer, int *offset)
{
	if (!dentry || dentry == dentry->d_parent) // 根目录终止条件
		return;

	build_path(dentry->d_parent, buffer, offset); // 递归父目录

	// 拼接当前目录名
	int len = snprintf(buffer + *offset, PATH_MAX - *offset, "/%s",
			   dentry->d_name.name);

	*offset += len;
}

char *get_absolute_path(struct dentry *d)
{
	int offset = 0;
	char *buffer = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buffer)
		return NULL;

	build_path(d, buffer, &offset);
	return buffer;
}

static int vfs_write_pre(struct kprobe *p, struct pt_regs *regs)
{
	int slen = 0;
	char *fname = NULL;
	char *data = NULL;
	char *fpath = NULL;

	// x86_64 参数顺序依次为 rdi, rsi, rdx, rcx
	struct file *file = (struct file *)regs->di;
	const char __user *buf = (const char __user *)regs->si;
	size_t count = regs->dx;
	loff_t *pos = (loff_t *)regs->cx;


	// 检查用户空间指针合法性（适配 5.19 set_fs 移除）
	// if (!__kernel_ok(buf, count)) {
	// 	pr_info("Illegal user buffer: %p\n", buf);
	// 	return -EFAULT;
	// }

	fname = (char *)file->f_path.dentry->d_name.name;
	if (strcmp(fname, "test.txt") != 0) {
		return 0;
	}

	if (IS_ERR_OR_NULL(buf))
		return -EFAULT;

	slen = strnlen_user(buf, PATH_MAX);
	if (!slen || slen > PATH_MAX)
		return -EFAULT;

	data = kzalloc(slen + 1, GFP_ATOMIC);
	if (PTR_ERR_OR_ZERO(data))
		return -EFAULT;

	if (copy_from_user(data, buf, count)) {
		kfree(data);
		return -EFAULT;
	}

	fpath = get_absolute_path(file->f_path.dentry);

	// 拦截逻辑：记录写入操作（示例）
	pr_info("Process %s writes %zu bytes (pos:%lld) to %s: %s\n",
		current->comm, count, *pos, fpath, data);

	kfree(data);
	kfree(fpath);

	return 0;
}

static void vfs_write_post(struct kprobe *p, struct pt_regs *regs,
			   unsigned long flags)
{
	char *fpath = NULL;
	char *fname = NULL;
	struct file *file = (struct file *)regs->di; // x86_64 参数顺序

	fpath = get_absolute_path(file->f_path.dentry);

	fname = (char *)file->f_path.dentry->d_name.name;
	if (strcmp(fname, "test.txt") != 0) {
		return;
	}

	pr_info("write %s post!!\n", fpath);
	kfree(fpath);
}

static int vfs_rename_pre(struct kprobe *p, struct pt_regs *regs)
{
	char *fname = NULL;
	char *newname = NULL;
	struct renamedata *rd = (struct renamedata *)regs->di;

	fname = (char *)rd->old_dentry->d_name.name;
	if (strcmp(fname, "test.txt") != 0) {
		return 0;
	}

	newname = (char *)rd->new_dentry->d_name.name;
	pr_info("rename %s -> %s\n", fname, newname);

	return 0;
}

static int vfs_unlink_pre(struct kprobe *p, struct pt_regs *regs)
{
	char *fpath = NULL;
	char *fname = NULL;
	struct dentry *d = (struct dentry *)regs->dx;

	fname = (char *)d->d_name.name;
	if (strcmp(fname, "test.txt") != 0) {
		return 0;
	}

	fpath = get_absolute_path(d);
	pr_info("remove %s\n", fpath);

	return 0;
}

static int __init hook_init(void)
{
	int i;
	int kps_count;
	struct kprobe vfs_write_sp = {
		.symbol_name = "vfs_write",
		.pre_handler = vfs_write_pre,
		.post_handler = vfs_write_post,
	};

	struct kprobe vfs_rename_sp = {
		.symbol_name = "vfs_rename",
		.pre_handler = vfs_rename_pre,
		.post_handler = NULL,
	};

	struct kprobe vfs_unlink_sp = {
		.symbol_name = "vfs_unlink",
		.pre_handler = vfs_unlink_pre,
	};

	kps[0] = vfs_write_sp;
	kps[1] = vfs_rename_sp;
	kps[2] = vfs_unlink_sp;

	kps_count = 3;
	for (i = 0; i < kps_count; i++) {
		register_kprobe(&kps[i]);
	}

	return 0;
}

static void __exit hook_exit(void)
{
	int i;
	int kps_count = 3;
	for (i = 0; i < kps_count; i++) {
		unregister_kprobe(&kps[i]);
	}
}

module_init(hook_init);
module_exit(hook_exit);

MODULE_LICENSE("GPL");