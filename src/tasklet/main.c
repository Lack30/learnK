#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

static struct tasklet_struct my_tasklet;
static int irq_num = 1; // 假设使用键盘中断（IRQ1）
static int irq_counter = 0;

// Tasklet 处理函数（软中断上下文）
void my_tasklet_handler(unsigned long data)
{
	printk(KERN_INFO "Tasklet running: data=%lu, jiffies=%lu\n", data, jiffies);

	// 模拟耗时操作（非阻塞方式）
	mdelay(5); // 注意：在软中断上下文只能用 mdelay（忙等待），不可用 msleep
}

// 中断处理函数（顶半部）
irqreturn_t irq_handler(int irq, void *dev_id)
{
	if (irq != irq_num)
		return IRQ_NONE;

	// 记录中断触发次数
	irq_counter++;

	// 调度 tasklet（将耗时操作延迟到底半部）
	tasklet_schedule(&my_tasklet);

	return IRQ_HANDLED;
}

// 模块初始化
static int __init tasklet_demo_init(void)
{
	// 初始化 tasklet（静态方式）
	tasklet_init(&my_tasklet, my_tasklet_handler, 0x1234);

	// 注册中断处理程序
	if (request_irq(irq_num, irq_handler, IRQF_SHARED, "my_tasklet_irq",
					&irq_num)) {
		printk(KERN_ERR "Failed to register IRQ %d\n", irq_num);
		return -EIO;
	}

	printk(KERN_INFO "Tasklet module loaded\n");
	return 0;
}

// 模块卸载
static void __exit tasklet_demo_exit(void)
{
	// 禁用并释放 tasklet
	tasklet_kill(&my_tasklet);

	// 释放中断号
	free_irq(irq_num, &irq_num);

	printk(KERN_INFO "Tasklet unloaded, total IRQs: %d\n", irq_counter);
}

module_init(tasklet_demo_init);
module_exit(tasklet_demo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lack30");