#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

static struct hrtimer my_hrtimer;
static ktime_t interval;

// 高精度定时器回调
enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
	printk(KERN_INFO "HRTimer callback executed (ns: %lld)\n", ktime_get_ns());

	// 重启定时器（纳秒级精度）
	hrtimer_forward_now(timer, interval);
	return HRTIMER_RESTART;
}

static int __init __hrtimer_init(void)
{
	printk(KERN_INFO "Initializing hrtimer module\n");

	// 设置定时间隔：500毫秒（500,000,000纳秒）
	interval = ktime_set(0, 500000000); // 0秒 + 500,000,000纳秒

	// 初始化并启动高精度定时器
	hrtimer_init(&my_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	my_hrtimer.function = hrtimer_callback;
	hrtimer_start(&my_hrtimer, interval, HRTIMER_MODE_REL);

	return 0;
}

static void __exit __hrtimer_exit(void)
{
	hrtimer_cancel(&my_hrtimer);
	printk(KERN_INFO "HRTimer module unloaded\n");
}

module_init(__hrtimer_init);
module_exit(__hrtimer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lack30");