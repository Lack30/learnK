#include "linux/dcache.h"
#include "linux/string.h"
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>

static inline unsigned long pt_regs_params(struct pt_regs *regs, int idx)
{
	unsigned long val;
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	val = (idx == 31) ? 0 : regs->regs[idx];
#else
	switch (idx) {
	case 0:
		val = regs->di;
		break;
	case 1:
		val = regs->si;
		break;
	case 2:
		val = regs->dx;
		break;
	case 3:
		val = regs->r10;
		break;
	case 4:
		val = regs->r8;
		break;
	default:
		val = 0;
		break;
	}
#endif

	return val;
}

struct mount_params {
	char dev_name[256];  // 设备路径
	char dir_name[256];  // 挂载点
	char fs_type[64];    // 文件系统类型
	unsigned long flags; // 挂载标志
};

// 自定义数据结构，包含红黑树节点
struct my_entry {
	struct rb_node node;	     // 必须嵌入红黑树节点
	unsigned long key;	     // 排序键值
	struct mount_params *params; // 挂载参数
};

struct probe_pool {
	struct rb_root root;
	spinlock_t lock;
};

void probe_pool_init(struct probe_pool *map)
{
	map->root = RB_ROOT;
	spin_lock_init(&map->lock);
}

void probe_pool_clear(struct probe_pool *map)
{
	unsigned long flags;
	struct rb_node *node;
	spin_lock_irqsave(&map->lock, flags);
	while ((node = rb_first(&map->root))) {
		struct my_entry *entry = rb_entry(node, struct my_entry, node);
		rb_erase(node, &map->root);
		kfree(entry);
	}
	spin_unlock_irqrestore(&map->lock, flags);
}

static struct probe_pool *probe_pool = NULL;

int insert_node(struct rb_root *my_tree, unsigned long key, struct mount_params *params)
{
	struct rb_node **new = &my_tree->rb_node;
	struct rb_node *parent = NULL;
	struct my_entry *entry;

	// 查找插入位置
	while (*new) {
		struct my_entry *cur = rb_entry(*new, struct my_entry, node);
		parent = *new;
		if (key < cur->key)
			new = &(*new)->rb_left;
		else if (key > cur->key)
			new = &(*new)->rb_right;
		else
			return -EEXIST; // 键值已存在
	}

	// 分配新节点
	entry = kmalloc(sizeof(struct my_entry), GFP_KERNEL);
	entry->key = key;
	entry->params = params;

	// 链接节点并调整颜色
	rb_link_node(&entry->node, parent, new);
	rb_insert_color(&entry->node, my_tree);
	return 0;
}

struct my_entry *search_node(struct rb_root *my_tree, unsigned long key)
{
	struct rb_node *n = my_tree->rb_node;
	while (n) {
		struct my_entry *cur = rb_entry(n, struct my_entry, node);
		if (key < cur->key)
			n = n->rb_left;
		else if (key > cur->key)
			n = n->rb_right;
		else
			return cur; // 找到匹配项
	}
	return NULL; // 未找到
}

struct mount_params *pop_node(struct rb_root *my_tree, unsigned long key)
{
	struct mount_params *params = NULL;
	struct my_entry *entry = search_node(my_tree, key);
	if (entry) {
		rb_erase(&entry->node, my_tree); // 从树中移除
		params = entry->params;
		kfree(entry); // 释放节点内存
	}
	return params;
}

int insert_params(struct probe_pool *map, unsigned long key, struct mount_params *params)
{
	unsigned long flags;
	spin_lock_irqsave(&map->lock, flags);
	insert_node(&map->root, key, params);
	spin_unlock_irqrestore(&map->lock, flags);
	return 0;
}

struct mount_params *pop_params(struct probe_pool *map, unsigned long key)
{
	unsigned long flags;
	struct mount_params *params = NULL;
	spin_lock_irqsave(&map->lock, flags);
	params = pop_node(&map->root, key);
	spin_unlock_irqrestore(&map->lock, flags);
	return params;
}

static void build_path(struct dentry *dentry, char *buffer, int *offset)
{
	if (!dentry || dentry == dentry->d_parent)
		return;

	build_path(dentry->d_parent, buffer, offset);

	*offset += snprintf(buffer + *offset, PATH_MAX - *offset, "/%s", dentry->d_name.name);
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

/*
 int path_mount(const char *dev_name, struct path *path,
                const char *type_page, unsigned long flags, void *data_page);

int ksys_mount(char __user *dev_name, char __user *dir_name, char __user *type,
               unsigned long flags, void __user *data);
 */

// 函数入口处理：记录参数
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	// 为当前实例分配参数存储结构体
	struct mount_params *params;
	params = kmalloc(sizeof(struct mount_params), GFP_KERNEL);
	unsigned long ptr = (unsigned long)ri;
	pr_info("[before] ri instance at %lx\n", ptr);

	if (!regs)
		return 0;

	const char *dev_name = (const char *)pt_regs_params(regs, 0);
	struct path *path = (struct path *)pt_regs_params(regs, 1);
	const char *type_path = (const char *)pt_regs_params(regs, 2);
	unsigned long flags = pt_regs_params(regs, 3);
	if (params && ptr > 0) {
		// 从寄存器中提取参数并保存到结构体
		strncpy(params->dev_name, dev_name, 256);
		strncpy(params->dir_name, get_absolute_path(path->dentry), 256);
		strncpy(params->fs_type, type_path, 64);
		params->flags = flags;
		insert_params(probe_pool, ptr, params); // 将参数存储到红黑树中
	}
	// 将结构体指针绑定到当前实例的 data 字段
	// ri->data = params;

	pr_info("[before] mount %s to %s\n", dev_name, params->dir_name);
	// strcpy(ri->data, dev_name); // 记录参数
	// pr_info("[before] ri->data = %s\n", ri->data);

	return 0;
}

// 函数返回处理：记录结果
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret;
	ret = (int)regs->ax;
	unsigned long ptr = (unsigned long)ri;
	struct mount_params *params = NULL;

	if (!regs)
		return 0;

	params = pop_params(probe_pool, ptr); // 从红黑树中获取参数

	pr_info("[after] mount ret: %d\n", ret);
	// pr_info("[after] ri->data = %s\n", ri->data);
	pr_info("[after] ri instance at %lx\n", ptr);
	if (params) {
		pr_info("[after] mount %s to %s\n", params->dev_name, params->dir_name);
		pr_info("[after] fs_type: %s\n", params->fs_type);
		pr_info("[after] flags: %lu\n", params->flags);
		kfree(params);
	}

	return 0;
}

static struct kretprobe my_kretprobe = {
	.kp.symbol_name = "path_mount",
	.handler = ret_handler,		// 返回时触发的回调
	.entry_handler = entry_handler, // 进入时触发的回调
	.maxactive = 8			// 根据并发需求调整
};

static int __init hook_init(void)
{
	int ret;
	probe_pool = kmalloc(sizeof(struct probe_pool), GFP_KERNEL);
	probe_pool_init(probe_pool);
	ret = register_kretprobe(&my_kretprobe);
	if (ret) {
		pr_err("register_kretprobe failed, returned %d\n", ret);
		return -EINVAL;
	}
	pr_info("%ld/%d=%d\n", PAGE_SIZE, SECTOR_SIZE, PAGE_SIZE / SECTOR_SIZE);
	pr_info("kretprobe registered\n");

	return 0;
}

static void __exit hook_exit(void)
{
	unregister_kretprobe(&my_kretprobe);
	probe_pool_clear(probe_pool);
	kfree(probe_pool);
	pr_info("kretprobe unregistered\n");
}

module_init(hook_init);
module_exit(hook_exit);

MODULE_LICENSE("GPL");
