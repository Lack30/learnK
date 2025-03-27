#include <linux/init.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/module.h>

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者
MODULE_DESCRIPTION("Learn linux List"); // 模块详细说明信息
MODULE_VERSION("0.01"); // 模块版本

struct Node {
	int value;
	struct list_head list;
};

// 模块注册函数，在模块被加载时调用
static int __init list_init(void)
{
	pr_info("======== initization linux list ========");
	struct list_head head;
	INIT_LIST_HEAD(&head); // 初始化

	if (list_empty(&head)) {
		pr_info("list empty");
	}

	pr_info("======== add n1 ========");

	struct Node n1;
	n1.value = 1;
	list_add_tail(&n1.list, &head); // 队尾插入

	pr_info("======== add n2 ========");

	struct Node n2;
	n2.value = 2;
	list_add(&n2.list, &head); // 队头插入

	pr_info("======== add n3 ========");

	struct Node n3;
	n3.value = 3;
	list_add_tail(&n3.list, &head);

	pr_info("======== add n4 ========");

	struct Node n4;
	n4.value = 4;
	list_add_tail(&n4.list, &head);

	pr_info("======== travese list ========");

	struct list_head *pos;
	list_for_each (pos, &head) {
		struct Node *n = list_entry(pos, struct Node, list);
		pr_info("n=%d\n", n->value);
	}

	pr_info("======== del n2 ========");

	list_del(&n2.list);

	pr_info("======== del n4 ========");

	list_del(&n4.list);

	pr_info("======== travese list ========");

	struct list_head *idx;
	list_for_each_safe (pos, idx, &head) {
		struct Node *n = list_entry(pos, struct Node, list);
		pr_info("n=%d\n", n->value);
		list_del(&n->list);
	}

	list_for_each (pos, &head) {
		struct Node *n = list_entry(pos, struct Node, list);
		pr_info("n=%d\n", n->value);
	}

	if (list_empty(&head)) {
		pr_info("list empty");
	}

	return 0;
}

// 模块注销函数，在模块被移除时调用
static void __exit list_exit(void)
{
}

module_init(list_init);
module_exit(list_exit);
