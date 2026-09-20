#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_parse.h"
#include "http_recv.h"
#include "http_response.h"


int main(void) {
    
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,     // IP 地址族, AF_UNSPEC 值代表不限制 IPv4/IPv6
        .ai_socktype = SOCK_STREAM, // socket 类型, SOCK_STREAM 值代表流式套接字(TCP)
        .ai_flags = AI_PASSIVE      // 地址用途, AI_PASSIVE 值代表用于 listen 的服务器地址
    };
    struct addrinfo *serverinfo;    // 用于指向 getaddrinfo 分配的地址等信息

    /** 寻找符合 hints 的地址信息链表, 使 serverinfo 指向第 1 个 */
    getaddrinfo(
        NULL,       // 域名或 IP 地址
        "3490",     // 端口号
        &hints,
        &serverinfo
    );

    /** 创建 socket, 用于 listen */
    int listenfd = socket(          
        serverinfo->ai_family,      
        serverinfo->ai_socktype,
        serverinfo->ai_protocol     // 协议族(TCP/UDP), 由 getaddrinfo 根据 hints 和查询结果分配
    );

    /** 将 listenfd 与 serverinfo 绑定 */
    bind(
        listenfd,
        serverinfo->ai_addr,    // 具体地址(IPv4/Ipv6), 由 getaddrinfo 根据 hints 和查询结果分配
        serverinfo->ai_addrlen  // 具体地址的字节长, 由 getaddrinfo 根据 hints 和查询结果分配
    );

    /** 后续不再使用 serverinfo, 使用专用函数释放 */
    freeaddrinfo(serverinfo);

    /** 开始监听客户端对已绑定的 ai_addr 与端口的连接请求 */
    listen(
        listenfd,
        5           // 允许"未被处理, 正在排队"的请求数
    );

    /** 循环接受请求 */
    while (1) {

        /** 阻塞, 接受 1 个连接请求后返回 1 个新的文件描述符, 用于后续 recv 和 send 等操作 */
        int connectfd = accept(
            listenfd,
            NULL,       // 用于接收客户端的地址等信息, 此处不接收
            NULL        // 用于记录客户端信息的字节长, 此处不记录
        );

        /**************** HTTP ****************/

        char buf[8192];                 // 用于存储完整 HTTP 请求报文的 buffer
        struct http_request request;    // 用于存储解析 HTTP 请求报文的结果

        /** 接收完整请求 bytes, 返回请求报文总长度 */
        size_t request_length = recv_http1_request(connectfd, buf, sizeof buf);

        /** 解析 HTTP 请求报文 */
        parse_http1_request(buf, request_length, &request);

        /** 向客户端发送 HTTP 响应报文 */
        send_http1_response(connectfd, &request);

        /**************************************/

        /** 发完就关 */
        close(connectfd);
    }
}