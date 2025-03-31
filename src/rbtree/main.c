#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rbtree.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

// 自定义数据结构，包含红黑树节点
struct my_entry {
	struct rb_node node; // 必须嵌入红黑树节点
	int key; // 排序键值
	char data[64]; // 自定义数据
};

static struct rb_root my_tree = RB_ROOT; // 红黑树根节点

int insert_node(int key, const char *data)
{
	struct rb_node **new = &my_tree.rb_node;
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
	strncpy(entry->data, data, sizeof(entry->data));

	// 链接节点并调整颜色
	rb_link_node(&entry->node, parent, new);
	rb_insert_color(&entry->node, &my_tree);
	return 0;
}

struct my_entry *search_node(int key)
{
	struct rb_node *n = my_tree.rb_node;
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

void delete_node(int key)
{
	struct my_entry *entry = search_node(key);
	if (entry) {
		rb_erase(&entry->node, &my_tree); // 从树中移除
		kfree(entry); // 释放内存
	}
}

void print_all_entries(void)
{
	struct rb_node *n;
	for (n = rb_first(&my_tree); n; n = rb_next(n)) {
		struct my_entry *entry = rb_entry(n, struct my_entry, node);
		printk(KERN_INFO "Key: %d, Data: %s\n", entry->key, entry->data);
	}
}

static int __init start_init(void)
{
	insert_node(100, "1 Entry");
	insert_node(200, "2 Entry");
	insert_node(150, "3 Entry");
	insert_node(220, "4 Entry");
	insert_node(300, "5 Entry");
	insert_node(110, "6 Entry");
	print_all_entries();

	return 0;
}
module_init(start_init);

static void __exit end_exit(void)
{
	struct rb_node *n;
	while ((n = rb_first(&my_tree))) { // 遍历释放所有节点
		struct my_entry *entry = rb_entry(n, struct my_entry, node);
		rb_erase(n, &my_tree);
		kfree(entry);
	}
}

module_exit(end_exit);