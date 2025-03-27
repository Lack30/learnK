#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/slab.h>

DECLARE_HASHTABLE(ht, 7);

struct my_data {
	int key;
	struct hlist_node node;
	char *name;
};

static void add_element(int key, char *name)
{
	struct my_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return;
	data->key = key;
	data->name = name;
	hash_add(ht, &data->node, key);
}

static void remove_element(struct my_data *data)
{
	hash_del(&data->node);
	kfree(data);
}

struct my_data *get_element(int key)
{
	struct my_data *ptr;
	hash_for_each_possible (ht, ptr, node, key) {
		if (ptr->key == key) {
			return ptr;
		}
	}
	return NULL;
}

static void clear(void)
{
	struct my_data *ptr;
	struct hlist_node *tmp;
	int btk = 0;
	hash_for_each_safe (ht, btk, tmp, ptr, node) {
		hash_del(&ptr->node);
	}
}

static void print_hash_table(void)
{
	struct my_data *ptr;
	int btk = 0;
	hash_for_each (ht, btk, ptr, node) {
		printk(KERN_INFO "Key: %d name: %s\n", ptr->key, ptr->name);
	}
}

static int __init my_init(void)
{
	pr_info("======== initization linux hashmap ========");
	hash_init(ht);

	pr_info("======== add elements ========");

	add_element(1, "John");
	add_element(2, "Bob");

	pr_info("======== print hashmap ========");

	print_hash_table();

	pr_info("======== remove element ========");
	struct my_data *p = get_element(1);
	if (p)
		remove_element(p);

	pr_info("======== print hashmap ========");

	print_hash_table();

	pr_info("======== clear hashmap ========");

	clear();

	return 0;
}
module_init(my_init);

static void __exit my_exit(void)
{
}
module_exit(my_exit);

MODULE_LICENSE("GPL");