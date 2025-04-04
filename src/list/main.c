#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/types.h>

#include "container.h"

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者
MODULE_DESCRIPTION("Learn linux List"); // 模块详细说明信息
MODULE_VERSION("0.01"); // 模块版本

// 模块注册函数，在模块被加载时调用
static int __init list_init(void)
{
	pr_info("======== LinkList ========");
	struct LinkList *l = add_link_list();

	link_list_push(l, 1);
	link_list_push(l, 2);
	link_list_push(l, 3);

	link_list_insert(l, 1, 4);
	link_list_remove(l, 2);

	char *buffer = NULL;
	buffer = to_string(&l->head);
	pr_info("link_list: %s\n", buffer);
	kfree(buffer);

	int v = 0;
	link_list_pop(l, &v);
	pr_info("print node=%d, length=%d\n", v, link_list_len(l));
	link_list_pop(l, &v);
	pr_info("print node=%d, length=%d\n", v, link_list_len(l));
	link_list_pop(l, &v);
	pr_info("print node=%d, length=%d\n", v, link_list_len(l));

	del_link_list(l);

	pr_info("======== Queue ========");
	struct Queue *q = add_queue();
	enqueue(q, 1);
	enqueue(q, 2);
	enqueue(q, 3);

	buffer = to_string(&q->head);
	pr_info("queue: %s\n", buffer);
	kfree(buffer);

	int i = 0;
	dequeue(q, &i);
	pr_info("print node=%d, length=%d\n", i, queue_len(q));
	dequeue(q, &i);
	pr_info("print node=%d, length=%d\n", i, queue_len(q));
	dequeue(q, &i);
	pr_info("print node=%d, length=%d\n", i, queue_len(q));

	del_queue(q);

	pr_info("======== Stack ========");

	struct Stack *s = add_stack();
	stack_push(s, 1);
	stack_push(s, 2);
	stack_push(s, 3);

	buffer = to_string(&s->head);
	pr_info("stack: %s\n", buffer);
	kfree(buffer);

	int a = 0;
	stack_pop(s, &a);
	pr_info("print node=%d, length=%d\n", a, stack_len(s));
	stack_pop(s, &a);
	pr_info("print node=%d, length=%d\n", a, stack_len(s));
	stack_pop(s, &a);
	pr_info("print node=%d, length=%d\n", a, stack_len(s));

	del_stack(s);

	return 0;
}

// 模块注销函数，在模块被移除时调用
static void __exit list_exit(void)
{
}

module_init(list_init);
module_exit(list_exit);
