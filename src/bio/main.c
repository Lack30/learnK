#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "asm/string_64.h"
#include "linux/mm_types.h"

#define SECTOR_SIZE 512 // 默认扇区大小（需根据实际设备确认）

static struct block_device *target_bdev = NULL;
static struct bio *custom_bio = NULL;
static char *read_buffer = NULL;
static char *write_buffer = NULL;

static void custom_bio_end_io(struct bio *bio)
{
	// 释放bio结构体
	bio_put(bio);
}

// 自定义Bio提交函数
static int submit_custom_bio(sector_t sector, int op)
{
    struct bio *bio = bio_alloc(GFP_KERNEL, 1);
    if (!bio)
        return -ENOMEM;

	bio->bi_iter.bi_sector = sector;
	bio->bi_bdev = target_bdev;
	bio->bi_opf = op;
	bio_set_dev(bio, target_bdev);
	bio->bi_end_io = custom_bio_end_io; // 设置自定义完成回调函数

    if (op == REQ_OP_READ)
    {
        struct page *vp = virt_to_page(read_buffer);
        if (!vp)
        {
            bio_put(bio);
            return -ENOMEM;
        }

        bio_add_page(bio, vp, SECTOR_SIZE, offset_in_page(read_buffer));
    }
    else
    {
        strcpy(write_buffer, "Hello, World!"); // 写入数据
        struct page *vp = virt_to_page(write_buffer);
        if (!vp)
        {
            bio_put(bio);
            return -ENOMEM;
        }
        bio_add_page(bio, vp, SECTOR_SIZE, offset_in_page(write_buffer));
    }

    pr_info("submit io: %lld\n", sector);

    submit_bio_wait(bio);
    return 0;
}

// 模块初始化
static int __init my_module_init(void)
{
    int ret = 0;
    // 获取块设备指针（需确保/dev/sdb存在且可访问）
    target_bdev =
            blkdev_get_by_path("/dev/sdb", FMODE_READ | FMODE_WRITE, NULL);
    if (IS_ERR(target_bdev))
    {
        printk(KERN_ERR "Failed to open /dev/sdb\n");
        return PTR_ERR(target_bdev);
    }

    // 分配内存缓冲区
    read_buffer = kmalloc(SECTOR_SIZE, GFP_KERNEL);
    write_buffer = kmalloc(SECTOR_SIZE, GFP_KERNEL);
    if (!read_buffer || !write_buffer)
    {
        ret = -ENOMEM;
        goto cleanup;
    }

    // 写第2扇区
    ret = submit_custom_bio(2, REQ_OP_WRITE);
    if (ret)
    {
        printk(KERN_ERR "Write failed\n");
        goto cleanup;
    }
    printk(KERN_INFO "Write sector 2 success\n");

    // 读第2扇区
    ret = submit_custom_bio(2, REQ_OP_READ);
    if (ret)
    {
        printk(KERN_ERR "Read failed\n");
        goto cleanup;
    }
    printk(KERN_INFO "Read sector 2: %s\n", read_buffer);

cleanup:
    if (target_bdev)
        blkdev_put(target_bdev, FMODE_READ | FMODE_WRITE);
    kfree(read_buffer);
    kfree(write_buffer);
    return ret;
}

// 模块卸载
static void __exit my_module_exit(void)
{
    printk(KERN_INFO "Module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");