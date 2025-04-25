#include <linux/init.h>
#include <linux/module.h>
#include "dumps.h"

static int __init __do_init(void)
{
	char *buf;
	int ret;
	struct params *target;
	struct params *p = dump_alloc(sizeof(struct params));
	p->id = 1;
	p->name = dump_alloc(32);
	strncpy(p->name, "hello world1", 32);

	buf = marshal(p);
	if (!buf) {
		printk(KERN_ERR "marshal failed\n");
		dump_free(p->name);
		dump_free(p);
		return -ENOMEM;
	} else {
		target = dump_alloc(sizeof(struct params));
		unmarshal(buf, target);
		pr_info("id: %d, name: %s\n", target->id, target->name); 
	}

	if (p)
		kfree(p);

	return -EINVAL;
}

// 模块注销函数，在模块被移除时调用
static void __exit __do_exit(void)
{
}

module_init(__do_init); // 注册模块启动函数
module_exit(__do_exit); // 注册模块注销函数

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者