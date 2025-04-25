package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"syscall"
)

const (
	NETLINK_USER = 22
	USER_MSG     = 29
	MAX_PLOAD    = 1024
)

type MyParam struct {
	ID    int32
	Name  [32]byte
	Value int32
}

func main() {
	// 创建 Netlink 套接字
	sockfd, err := syscall.Socket(syscall.AF_NETLINK, syscall.SOCK_RAW, USER_MSG)
	if err != nil {
		fmt.Printf("create socket failure! %v\n", err)
		os.Exit(1)
	}
	defer syscall.Close(sockfd)

	// 本地地址
	local := &syscall.SockaddrNetlink{
		Family: syscall.AF_NETLINK,
		Pid:    uint32(os.Getpid()),
		Groups: 0,
	}

	// 绑定本地地址
	err = syscall.Bind(sockfd, local)
	if err != nil {
		fmt.Printf("bind() error! %v\n", err)
		os.Exit(1)
	}

	// 远程地址
	remote := &syscall.SockaddrNetlink{
		Family: syscall.AF_NETLINK,
		Pid:    0,
		Groups: 0,
	}

	// 构造消息
	data := MyParam{
		ID:    1,
		Value: 10,
	}
	copy(data.Name[:], "test")

	nlh := &syscall.NlMsghdr{
		Len:   syscall.NLMSG_HDRLEN + uint32(binary.Size(data)),
		Type:  0,
		Flags: 0,
		Seq:   0,
		Pid:   local.Pid,
	}

	// 将消息序列化
	buf := new(bytes.Buffer)
	err = binary.Write(buf, binary.LittleEndian, nlh)
	if err != nil {
		fmt.Printf("binary write error: %v\n", err)
		os.Exit(1)
	}
	err = binary.Write(buf, binary.LittleEndian, data)
	if err != nil {
		fmt.Printf("binary write error: %v\n", err)
		os.Exit(1)
	}

	// 发送消息
	fmt.Println("sendmsg start....")
	err = syscall.Sendto(sockfd, buf.Bytes(), 0, remote)
	if err != nil {
		fmt.Printf("send to kernel failure! %v\n", err)
		os.Exit(1)
	}

	// 接收消息
	fmt.Println("recvmsg start....")
	recvBuf := make([]byte, syscall.Getpagesize())
	n, _, err := syscall.Recvfrom(sockfd, recvBuf, 0)
	if err != nil {
		fmt.Printf("recv from kernel failure! %v\n", err)
		os.Exit(1)
	}

	// 解析接收到的消息
	recvData := recvBuf[syscall.NLMSG_HDRLEN:n]
	fmt.Printf("recv data: %s\n", string(recvData))
}
