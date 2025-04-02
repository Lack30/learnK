# 编译内核模块
使用的操作系统为 Ubuntu22，内核版本 5.15

```bash
make
```

生成文件 hook.ko

```bash
insmod hook.ko

# rmmod hook.ko
```

# 检查结果

```bash
dmesg -T -w

go run test.go -pos=2 -data="hello"
```