#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>

static struct timer_list my_timer;

// 定时器回调函数
void timer_callback(struct timer_list *t)
{
	printk(KERN_INFO "Timer callback executed (jiffies: %lu)\n", jiffies);

	// 重启定时器（周期性触发）
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000)); // 2秒后再次触发
}

static int __init timer_init(void)
{
	printk(KERN_INFO "Initializing timer module\n");

	// 初始化定时器并绑定回调
	timer_setup(&my_timer, timer_callback, 0);

	// 设置首次触发时间：2秒后
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	return 0;
}

static void __exit timer_exit(void)
{
	del_timer_sync(&my_timer);
	printk(KERN_INFO "Timer module unloaded\n");
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lack30");