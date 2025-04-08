#include <linux/interval_tree.h>
#include <linux/module.h>
#include <linux/slab.h>

struct NodeValue
{
    struct interval_tree_node node;
    int data;
};

static struct rb_root_cached root = RB_ROOT_CACHED;

static struct NodeValue *search(struct rb_root_cached *root,
                                unsigned long start, unsigned long last)
{
    struct NodeValue *nv;
    struct interval_tree_node *node;
    for (node = interval_tree_iter_first(root, start, last); node;
         node = interval_tree_iter_next(node, start, last))
    {
        nv = container_of(node, struct NodeValue, node);
    }

    return nv;
}

static void insert(struct rb_root_cached *root, unsigned long start,
				   unsigned long last, int data)
{
	struct NodeValue *nv = kmalloc(sizeof(*nv), GFP_KERNEL);
	if (!nv)
		return;

	nv->node.start = start;
	nv->node.last = last;
	nv->data = data;

	interval_tree_insert(&nv->node, root);
}

static void remove(struct rb_root_cached *root, unsigned long start,
				   unsigned long last)
{
	struct NodeValue *nv = search(root, start, last);
	if (nv) {
		interval_tree_remove(&nv->node, root);
		kfree(nv);
	}
}

static void remove_all(struct rb_root_cached *root)
{
	struct NodeValue *nv;
	struct interval_tree_node *node;

	while ((node = interval_tree_iter_first(root, 0, ULONG_MAX))) {
		nv = container_of(node, struct NodeValue, node);
		interval_tree_remove(&nv->node, root);
		kfree(nv);
	}
}

static int interval_tree_test_init(void)
{
	printk(KERN_INFO "=== interval tree init ===\n");

    insert(&root, 0, 10, 1);
	insert(&root, 10, 20, 2);
	insert(&root, 20, 30, 3);
	insert(&root, 30, 40, 4);
	insert(&root, 40, 50, 5);

	remove(&root, 9, 15);

	struct NodeValue *nv1 = search(&root, 1, 10);
	if (nv1)
		printk(KERN_ALERT "Found node with data: %d\n", nv1->data);
	struct NodeValue *nv2 = search(&root, 15, 25);
	if (nv2)
		printk(KERN_ALERT "Found node with data: %d\n", nv2->data);
	struct NodeValue *nv3 = search(&root, 45, 55);
	if (nv3)
		printk(KERN_ALERT "Found node with data: %d\n", nv3->data);

	struct NodeValue *nv4 = search(&root, 65, 75);
	if (nv4)
		printk(KERN_ALERT "Found node with data: %d\n", nv4->data);

    return -EAGAIN; /* Fail will directly unload the module */
}

static void interval_tree_test_exit(void)
{
    printk(KERN_ALERT "test exit\n");
}

module_init(interval_tree_test_init);
module_exit(interval_tree_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michel Lespinasse");
MODULE_DESCRIPTION("Interval Tree test");