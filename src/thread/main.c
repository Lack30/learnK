#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>

static struct task_struct *demo_task;
// 定义工作队列和工作结构体
static struct workqueue_struct *demo_workqueue;
static struct work_struct demo_work;

// 工作队列处理函数
static void workqueue_function(struct work_struct *work)
{
    printk(KERN_INFO "Work queue function running...\n");
}

// 线程函数
static int thread_function(void *data)
{
    while (!kthread_should_stop())
    {
        printk(KERN_INFO "Thread running...\n");
        msleep(1000); // 休眠1秒
    }
    return 0;
}

// 模块初始化
static int __init thread_demo_init(void)
{
    // 创建工作队列
    demo_workqueue = create_workqueue("demo_workqueue");
    if (!demo_workqueue)
    {
        printk(KERN_ERR "Failed to create workqueue\n");
        return -ENOMEM;
    }

    // 初始化工作结构体并将其加入工作队列
    INIT_WORK(&demo_work, workqueue_function);
    queue_work(demo_workqueue, &demo_work);

    // 创建线程
    demo_task = kthread_run(thread_function, NULL, "demo_thread");
    if (IS_ERR(demo_task))
    {
        destroy_workqueue(demo_workqueue);
        printk(KERN_ERR "Failed to create thread\n");
        return PTR_ERR(demo_task);
    }

    return 0;
}

// 模块卸载
static void __exit thread_demo_exit(void)
{
    // 停止线程
    if (demo_task)
    {
        kthread_stop(demo_task);
        printk(KERN_INFO "Thread stopped\n");
    }

    // 销毁工作队列
    if (demo_workqueue)
    {
        flush_workqueue(demo_workqueue);
        destroy_workqueue(demo_workqueue);
        printk(KERN_INFO "Workqueue destroyed\n");
    }
}

module_init(thread_demo_init);
module_exit(thread_demo_exit);
MODULE_LICENSE("GPL");