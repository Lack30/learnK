#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define FIFO_SIZE 10

struct fifo_item {
	int data;
	struct work_struct work;
};

static struct fifo_item *fifo[FIFO_SIZE];
static int fifo_head = 0;
static int fifo_tail = 0;
static struct mutex fifo_lock;
static struct workqueue_struct *workqueue;

static void work_handler(struct work_struct *work) {
	struct fifo_item *item = container_of(work, struct fifo_item, work);
	pr_info("Processing item with data: %d\n", item->data);
	kfree(item);
}

static int enqueue(int data) {
	struct fifo_item *item;

	mutex_lock(&fifo_lock);

	if ((fifo_tail + 1) % FIFO_SIZE == fifo_head) {
		pr_err("FIFO is full\n");
		mutex_unlock(&fifo_lock);
		return -1;
	}

	item = kmalloc(sizeof(*item), GFP_KERNEL);
	if (!item) {
		pr_err("Failed to allocate memory for FIFO item\n");
		mutex_unlock(&fifo_lock);
		return -1;
	}

	item->data = data;
	INIT_WORK(&item->work, work_handler);

	fifo[fifo_tail] = item;
	fifo_tail = (fifo_tail + 1) % FIFO_SIZE;

	queue_work(workqueue, &item->work);

	mutex_unlock(&fifo_lock);
	return 0;
}

static int __init workfifo_init(void) {
	pr_info("Initializing workfifo module\n");

	mutex_init(&fifo_lock);
	workqueue = create_singlethread_workqueue("workfifo_queue");
	if (!workqueue) {
		pr_err("Failed to create workqueue\n");
		return -ENOMEM;
	}

	enqueue(1);
	enqueue(2);
	enqueue(3);

	return 0;
}

static void __exit workfifo_exit(void) {
	struct fifo_item *item;

	pr_info("Exiting workfifo module\n");

	flush_workqueue(workqueue);
	destroy_workqueue(workqueue);

	mutex_lock(&fifo_lock);
	while (fifo_head != fifo_tail) {
		item = fifo[fifo_head];
		fifo_head = (fifo_head + 1) % FIFO_SIZE;
		kfree(item);
	}
	mutex_unlock(&fifo_lock);
}

module_init(workfifo_init);
module_exit(workfifo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple workfifo kernel module");