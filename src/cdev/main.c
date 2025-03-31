#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Lack30");
MODULE_LICENSE("GPL");

#define HELLODEV_SIZE 0x1000
#define MEM_CLEAR 0x1
#define HELLODEV_MAJOR 230

static int hello_major = HELLODEV_MAJOR;
module_param(hello_major, int, S_IRUGO);

struct hello_dev {
	struct cdev cdev;
	unsigned char mem[HELLODEV_SIZE];
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
		memset(dev->mem, 0, HELLODEV_SIZE);
		printk(KERN_INFO "hello is set to zero\n");
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static ssize_t hello_read(struct file *filp, char __user *buf, size_t size,
						  loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct hello_dev *dev = filp->private_data;

	if (p >= HELLODEV_SIZE)
		return 0;
	if (count > HELLODEV_SIZE - p)
		count = HELLODEV_SIZE - p;

	if (copy_to_user(buf, dev->mem + p, count)) {
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;

		printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
	}

	return ret;
}

static ssize_t hello_write(struct file *filp, const char __user *buf,
						   size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct hello_dev *dev = filp->private_data;

	if (p >= HELLODEV_SIZE)
		return 0;
	if (count > HELLODEV_SIZE - p)
		count = HELLODEV_SIZE - p;

	if (copy_from_user(dev->mem + p, buf, count))
		ret = -EFAULT;
	else {
		*ppos += count;
		ret = count;

		printk(KERN_INFO "written %u bytes(s) from %lu\n", count, p);
	}

	return ret;
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
