//go:build linux

// main.go
package main

import (
	"fmt"
	"syscall"
	"unsafe"
)

const (
	NETLINK_USER = 30
	MAX_PAYLOAD    = 1024
)

func main() {
	// 创建Netlink socket
	sock, err := syscall.Socket(syscall.AF_NETLINK, syscall.SOCK_RAW, NETLINK_USER)
	if err != nil {
		panic(err)
	}
	fmt.Printf("sock = %d\n", sock)

	// 绑定到自定义协议
	local := &syscall.SockaddrNetlink{
		Family: syscall.AF_NETLINK,
		Pid:    uint32(52),
		Groups: 0,
	}
	if err := syscall.Bind(sock, local); err != nil {
		panic(err)
	}

	// 构造发送消息
	msg := []byte("Hello from 11userspace!")
	nlh := syscall.NlMsghdr{
		Len:   syscall.NLMSG_HDRLEN + uint32(len(msg)),
		Type:  0,
		Flags: 0,
		Seq:   0,
		Pid:   uint32(52),
	}
	buf := make([]byte, nlh.Len)

	*(*syscall.NlMsghdr)(unsafe.Pointer(&buf[0])) = nlh
	copy(buf[syscall.NLMSG_HDRLEN:], msg)

	remote := &syscall.SockaddrNetlink{
		Family: syscall.AF_NETLINK,
		Pid:    uint32(0),
		Groups: 0,
	}

	// 发送消息
	fmt.Printf("send data: [%s] to kernel\n", string(buf))
	if err := syscall.Sendto(sock, buf, 0, remote); err != nil {
		panic(err)
	}

	// 接收响应
	resp := make([]byte, MAX_PAYLOAD)
	n, _, err := syscall.Recvfrom(sock, resp, 0)
	if err != nil {
		panic(err)
	}
	fmt.Printf("n = %d\n", n)

	// 解析响应
	nlh = *(*syscall.NlMsghdr)(unsafe.Pointer(&resp[0]))
	payload := string(resp[syscall.NLMSG_HDRLEN :])
	fmt.Printf("Received from kernel: %s\n", payload)
}
