#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <net/sock.h>

// unit参数对应协议类型，范围是0到31，其中0-15已被内核使用，16-31供用户自定义
// 使用 cat /proc/net/netlink | awk '{print $2}' | sort | uniq 查看是否被占用
#define USER_MSG 30
#define USER_PORT 50

static int stringlen(char *buf);

static int send_msg(char *pbuf, int pid);
static void recv_cb(struct sk_buff *skb);
static int nl_bind(struct net *net, int group);

static struct sock *netlink_fd = NULL;

struct netlink_kernel_cfg cfg = {
    .groups = 1,
    .flags = 0,
    .input = recv_cb,
    .bind = nl_bind,
};

static int __init test_netlink_init(void)
{
    printk("init netlink!\n");

    netlink_fd = netlink_kernel_create(&init_net, USER_MSG, &cfg);
    if (!netlink_fd)
    {
        printk(KERN_ERR "can not create a netlink socket!\n");
        return -1;
    }

    printk("netlink init successful!\n");
    return 0;
}

static void __exit test_netlink_exit(void)
{
    printk("exit netlink!\n");
    netlink_kernel_release(netlink_fd);
    printk(KERN_DEBUG "netlink exit!\n");
}

module_init(test_netlink_init);
module_exit(test_netlink_exit);

MODULE_AUTHOR("SSYFJ");
MODULE_LICENSE("GPL");

static int nl_bind(struct net *net, int group)
{
    pr_info("bind from %d\n", group);
    return 0;
}

static void recv_cb(struct sk_buff *__skb)
{
    struct nlmsghdr *nlh = NULL;
    void *data = NULL;
    int pid;

    struct sk_buff *skb =
            skb_get(__skb); //通过为原始数据__skb添加引用来获取数据

    printk("skb->len:%u\n", skb->len);
    if (skb->len >= NLMSG_SPACE(0)) //NLMSG_SPACE(0)表示  只有头部的大小
    {
        nlh = nlmsg_hdr(skb);
        printk("nlmsg->len:%u %u\n", nlh->nlmsg_len, nlmsg_len(nlh));
        data = NLMSG_DATA(nlh);
        if (data)
        {
            printk(KERN_INFO "kernel receive data [%d] [seq:%d]:%s\n",
                   nlh->nlmsg_pid, nlh->nlmsg_seq, (int8_t *)data);
            pid = nlh->nlmsg_pid;
            send_msg("hello userspace", pid);
        }
    }
    
    kfree_skb(skb); //前面引用+1,这里一定要释放，不然会warn
}

static int stringlen(char *buf)
{
    int len = 0;
    for (; *buf; buf++)
    {
        len++;
    }
    return len;
}

static int send_msg(char *pbuf, int pid)
{
    int len = stringlen(pbuf), ret;

    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;

    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if (!nl_skb)
    {
        printk("netlink_alloc_skb error!\n");
        return -1;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, pid, len, 0);
    if (!nlh)
    {
        printk("nlmsg_put() error!\n");
        nlmsg_free(nl_skb);
        return -1;
    }

    memcpy(nlmsg_data(nlh), pbuf, len);

    ret = netlink_unicast(netlink_fd, nl_skb, pid, MSG_DONTWAIT);
    // nlmsg_free(nl_skb);

    return ret;
}