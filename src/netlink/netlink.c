#include <errno.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NETLINK_USER 22
#define USER_MSG 30

#define MAX_PLOAD 1024

int main(int argc, char **argv)
{
    char *data = "hello kernel space by user 21";

    int sockfd, ret;
    struct sockaddr_nl local, remote;

    struct nlmsghdr *nlh = NULL;

    sockfd = socket(AF_NETLINK, SOCK_RAW, USER_MSG);
    if (sockfd == -1)
    {
        printf("create socket failure! %s\n", strerror(errno));
        return -1;
    }

    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_pid = 50;
    local.nl_groups = 0;
    if (bind(sockfd, (struct sockaddr *)&local, sizeof(local)) != 0)
    {
        printf("bind() error!\n");
        close(sockfd);
        return -1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.nl_family = AF_NETLINK;
    remote.nl_pid = 0;
    remote.nl_groups = 0;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
    memset(nlh, 0, sizeof(struct nlmsghdr));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_type = 0;
    nlh->nlmsg_seq = 0;
    nlh->nlmsg_pid = local.nl_pid;

    printf("sendmsg start....\n");

    memcpy(NLMSG_DATA(nlh), data, strlen(data));
    ret = sendto(sockfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&remote,
                 sizeof(struct sockaddr_nl));

    if (ret < 0)
    {
        perror("send to kernel failure!\n");
        close(sockfd);
        exit(-1);
    }

    //接受数据
    printf("recvmsg start....\n");

    memset(nlh, 0, NLMSG_SPACE(MAX_PLOAD));
    ret = recvfrom(sockfd, nlh, NLMSG_LENGTH(MAX_PLOAD), 0, NULL, NULL);
    if (ret < 0)
    {
        printf("recv from kernel failure!\n");
        close(sockfd);
        exit(-1);
    }

    printf("recv data:%s\n", (char *)NLMSG_DATA(nlh));

    close(sockfd);
    free((void *)nlh);

    return 0;
}