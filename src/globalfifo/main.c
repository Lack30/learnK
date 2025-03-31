#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/poll.h>

MODULE_AUTHOR("Lack30");
MODULE_LICENSE("GPL");

#define HELLODEV_SIZE 0x1000
#define MEM_CLEAR 0x1
#define HELLODEV_MAJOR 230

static int hello_major = HELLODEV_MAJOR;
module_param(hello_major, int, S_IRUGO);

struct hello_dev {
	struct cdev cdev;
	unsigned int current_len;
	unsigned char mem[HELLODEV_SIZE];
	struct mutex mutex;
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
};

struct hello_dev *hello_devp;

static int hello_open(struct inode *inode, struct file *filp)
{
	filp->private_data = hello_devp;
	return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long hello_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct hello_dev *dev = filp->private_data;

	switch (cmd) {
	case MEM_CLEAR:
		mutex_lock(&dev->mutex);
		memset(dev->mem, 0, HELLODEV_SIZE);
		mutex_unlock(&dev->mutex);
		printk(KERN_INFO "hello is set to zero\n");
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static ssize_t hello_read(struct file *filp, char __user *buf, size_t count,
						  loff_t *ppos)
{
	int ret;
	struct hello_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->r_wait, &wait);

	while (dev->current_len == 0) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);

		schedule();
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}

		mutex_lock(&dev->mutex);
	}

	if (count > dev->current_len)
		count = dev->current_len;

	if (copy_to_user(buf, dev->mem, count)) {
		ret = -EFAULT;
		goto out;
	} else {
		memcpy(dev->mem, dev->mem + count, dev->current_len - count);
		dev->current_len -= count;
		pr_info("read %ld byte(s) current_len: %d\n", count, dev->current_len);

		wake_up_interruptible(&dev->w_wait);

		ret = count;
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return ret;
}

static ssize_t hello_write(struct file *filp, const char __user *buf,
						   size_t count, loff_t *ppos)
{
	struct hello_dev *dev = filp->private_data;
	int ret;
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->w_wait, &wait);

	while (dev->current_len == HELLODEV_SIZE) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);

		mutex_unlock(&dev->mutex);

		schedule();
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}

		mutex_lock(&dev->mutex);
	}

	if (count > HELLODEV_SIZE - dev->current_len)
		count = HELLODEV_SIZE - dev->current_len;

	if (copy_from_user(dev->mem + dev->current_len, buf, count)) {
		ret = -EFAULT;
		goto out;
	} else {
		dev->current_len += count;
		pr_info("written %ld byte(s), current_len: %d\n", count,
				dev->current_len);

		wake_up_interruptible(&dev->r_wait);

		ret = count;
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);

	return ret;
}

static unsigned int hello_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct hello_dev *dev = filp->private_data;

	mutex_lock(&dev->mutex);

	poll_wait(filp, &dev->r_wait, wait);
	poll_wait(filp, &dev->w_wait, wait);

	if (dev->current_len != 0) {
		mask |= POLLIN | POLLRDNORM;
	}

	if (dev->current_len != HELLODEV_SIZE) {
		mask |= POLLOUT | POLLWRNORM;
	}

	mutex_lock(&dev->mutex);
	return mask;
}

static loff_t hello_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;
	switch (orig) {
	case 0:
		if (offset < 0) {
			ret = -EINVAL;
			break;
		}
		if ((unsigned int)offset > HELLODEV_SIZE) {
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:
		if ((filp->f_pos + offset) > HELLODEV_SIZE) {
			ret = -EINVAL;
			break;
		}
		if ((filp->f_pos + offset) < 0) {
			ret = -EINVAL;
			break;
		}
		filp->f_pos += offset;
		ret = filp->f_pos;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.llseek = hello_llseek,
	.read = hello_read,
	.write = hello_write,
	.poll = hello_poll,
	.unlocked_ioctl = hello_ioctl,
	.open = hello_open,
	.release = hello_release,
};

static void hello_setup_cdev(struct hello_dev *dev, int index)
{
	int err, devno = MKDEV(hello_major, index);

	cdev_init(&dev->cdev, &hello_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding hello%d", err, index);
}

static int __init hello_init(void)
{
	int ret;
	dev_t devno = MKDEV(hello_major, 0);

	if (hello_major)
		ret = register_chrdev_region(devno, 1, "hellodev");
	else {
		ret = alloc_chrdev_region(&devno, 0, 1, "hellodev");
		hello_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;

	hello_devp = kzalloc(sizeof(struct hello_dev), GFP_KERNEL);
	if (!hello_devp) {
		ret = -ENOMEM;
		goto fail_malloc;
	}

	hello_setup_cdev(hello_devp, 0);

	mutex_init(&hello_devp->mutex);
	init_waitqueue_head(&hello_devp->r_wait);
	init_waitqueue_head(&hello_devp->w_wait);

	return 0;

fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
}
module_init(hello_init);

static void __exit hello_exit(void)
{
	cdev_del(&hello_devp->cdev);
	kfree(hello_devp);
	unregister_chrdev_region(MKDEV(hello_major, 0), 1);
}
module_exit(hello_exit);
