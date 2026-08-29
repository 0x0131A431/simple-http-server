# Simple HTTP Server

## 项目功能

在放行 3490 端口流量的 Linux 环境机器上编译运行:
```bash
$ cd server

$ gcc -I include main.c http/http_parse.c http/http_response.c http/http_recv.c -o server

$ ./server
```

浏览器访问 `[IP]:3490`, 页面能成功返回 `Hello World!`

## 为什么要做这个

想要回归基本功, 使用 C, 从 0 去理解 HTTP 请求与响应的具体过程

搜索各种仓库或源码, 虽然资料丰富, 但是各种封装看的眼花

读源码十分枯燥, 还要从封装的函数拆出基本的请求逻辑, 耗时且打击信心

我只是想要一个极简的 HTTP 服务器

## 项目结构

实现了 HTTP 服务端的基本流程: 创建 socket, 绑定端口, 监听连接, 接收字节流, 解析 HTTP 报文和发送响应
```
getaddrinfo -> socket -> bind -> freeaddrinfo -> listen -> accept -> recv -> parse -> send -> close
```

```text
server
    ├── main.c                  # 服务端入口: 创建 socket, 监听并处理连接
    │
    ├── http
    │   ├── http_recv.c         # 负责接收完整 HTTP 请求报文
    │   ├── http_parse.c        # HTTP 请求数据结构与解析接口
    │   └── http_response.c     # HTTP 响应发送接口
    │
    └── include                 # 头文件声明
        ├── http_recv.h
        ├── http_parse.h
        └── http_response.h
```

## 注意

本项目生产意义不大, 实际是一份学习笔记, 为突出 HTTP 基本流程, 刻意省略了：

- 单线程, 串行处理连接, 不支持并发
- 只返回固定的 `Hello World!`, 没有路由, 静态文件和业务逻辑
- 只实现最基础的 HTTP/1.x 收发, 不支持 Keep-Alive, HTTP/2 等
- 请求缓冲区固定为 8 KB, 请求头最多保存 32 个, 超限处理未实现
- 没有错误处理和输入校验
