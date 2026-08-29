#include "http_response.h"

#include <sys/socket.h>

void send_http1_response(int sockfd) {

    /** 构造 HTTP 响应报文 */
    static const char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!";

    size_t sent = 0;                                // 记录已发送字节数
    size_t response_length = sizeof response - 1;   // 目标发送的总字节数

    /** 循环直至完整发送 */
    while (sent < response_length) {

        /** 发送数据, 返回此次发送 bytes 的长度 */
        sent += (size_t)send(
            sockfd,
            response + sent,        // 指定本次发送的 bytes 起点
            response_length - sent, // 指定本次发送的最大字节数
            0                       // flags, 无特殊处理
        );
    }
}
