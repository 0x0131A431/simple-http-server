#include "http_response.h"

#include <string.h>
#include <sys/socket.h>

void send_http1_response(int sockfd, struct http_request *request) {

    const char *response;

    /** 构造 HTTP 响应报文 */
    static const char ok_response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!";

    static const char not_found_response[] =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 9\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Not Found";

    if (strcmp(request->target, "/") == 0 || strcmp(request->target, "/hello") == 0) {
        response = ok_response;
    }
    else {
        response = not_found_response;
    }

    size_t sent = 0;                                // 记录已发送字节数
    size_t response_length = strlen(response);      // 目标发送的总字节数

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
