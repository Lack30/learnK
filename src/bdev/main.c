#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#define RAMBLOCK_SIZE (16 * 1024 * 1024) // 16MB 内存块设备
#define __SECTOR_SIZE 512

static struct gendisk *ramblock_disk;
static struct blk_mq_tag_set tag_set;
static unsigned char *ramblock_data;

// 请求处理函数（修复边界检查）
static blk_status_t
ramblock_process_request(struct blk_mq_hw_ctx *hctx,
                         const struct blk_mq_queue_data *bd) {
  struct request *req = bd->rq;
  blk_status_t status = BLK_STS_OK;

  unsigned long start_sector = blk_rq_pos(req);
  unsigned int nr_sectors = blk_rq_sectors(req);
  unsigned int offset = start_sector * __SECTOR_SIZE;
  struct bio_vec bvec;
  struct req_iterator iter;

  // 检查请求是否越界（关键修复点）
  if (start_sector + nr_sectors > get_capacity(ramblock_disk)) {
    status = BLK_STS_IOERR;
    goto end_request;
  }

  if (blk_rq_is_passthrough(req)) {
    pr_info("Skip non-fs request\n");
    status = BLK_STS_IOERR;
    goto end_request;
  }

  // 处理每个 bio 段
  rq_for_each_segment(bvec, req, iter) {
    void *buffer = kmap(bvec.bv_page) + bvec.bv_offset;
    int length = bvec.bv_len;
    if (offset + length > RAMBLOCK_SIZE) {
      kunmap(bvec.bv_page);
      status = BLK_STS_IOERR;
      goto end_request;
    }

    char *data = vmalloc(length);

    if (req_op(req) == REQ_OP_READ) {
      memcpy(buffer, ramblock_data + offset, length);

      memcpy(data, buffer, length);
      pr_info("read data (%d,%d): %s|\n", offset, length, data);

    } else if (req_op(req) == REQ_OP_WRITE) {
      memcpy(ramblock_data + offset, buffer, length);

      memcpy(data, buffer, length);
      pr_info("write data (%d,%d): %s|\n", offset, length, data);
    }

    vfree(data);

    kunmap(bvec.bv_page);
    offset += length;
  }

end_request:
  blk_mq_end_request(req, status);
  return status;
}

// 块设备操作函数
static const struct block_device_operations ramblock_fops = {
    .owner = THIS_MODULE,
};

static struct blk_mq_ops my_queue_ops = {
    .queue_rq = ramblock_process_request,
};

// 初始化块设备
static int __init ramblock_init(void) {
  // 分配内存（失败直接返回错误）
  ramblock_data = vmalloc(RAMBLOCK_SIZE);
  if (!ramblock_data)
    return -ENOMEM;

  // 配置 MQ 标签集
  memset(&tag_set, 0, sizeof(tag_set));
  tag_set.ops = &my_queue_ops;
  tag_set.nr_hw_queues = 1;
  tag_set.queue_depth = 128;
  tag_set.numa_node = NUMA_NO_NODE;
  tag_set.cmd_size = 0;
  tag_set.flags = BLK_MQ_F_SHOULD_MERGE;

  if (blk_mq_alloc_tag_set(&tag_set) != 0) {
    vfree(ramblock_data);
    return -ENOMEM;
  }

  // 创建磁盘对象
  ramblock_disk = blk_mq_alloc_disk(&tag_set, NULL);
  if (IS_ERR(ramblock_disk)) {
    blk_mq_free_tag_set(&tag_set);
    vfree(ramblock_data);
    return PTR_ERR(ramblock_disk);
  }

  // 设置磁盘属性（限制最大请求扇区）
  strcpy(ramblock_disk->disk_name, "ramdev");
  ramblock_disk->major = 0;
  ramblock_disk->first_minor = 0;
  ramblock_disk->fops = &ramblock_fops;
  set_capacity(ramblock_disk, RAMBLOCK_SIZE / __SECTOR_SIZE);

  ramblock_disk->queue = blk_mq_init_queue(&tag_set);
  if (IS_ERR(ramblock_disk->queue)) {
    blk_mq_free_tag_set(&tag_set);
    return -ENOMEM;
  }

  blk_queue_logical_block_size(ramblock_disk->queue, __SECTOR_SIZE);
  blk_queue_max_hw_sectors(ramblock_disk->queue, RAMBLOCK_SIZE / __SECTOR_SIZE);

  ramblock_disk->queue->queuedata = ramblock_disk;

  // 注册磁盘
  add_disk(ramblock_disk);
  pr_info("add device!!\n");
  return 0;
}

// 清理模块
static void __exit ramblock_exit(void) {
  if (ramblock_disk) {
    del_gendisk(ramblock_disk);
    put_disk(ramblock_disk);
  }
  blk_mq_free_tag_set(&tag_set);
  vfree(ramblock_data);
}

module_init(ramblock_init);
module_exit(ramblock_exit);

MODULE_LICENSE("GPL");