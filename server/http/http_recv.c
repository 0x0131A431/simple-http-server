#include "http_recv.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>


/**
 * @brief 解析请求头中的 `Content-Length`, 到此请求行和请求头已经获取完毕
 *
 * @param buf 指向 HTTP 请求报文起始位置的指针
 * @param headers_end 指向请求头结束位置的指针
 * @return 返回 Content-Length 的字段值
 */
static size_t get_content_length(char *buf, char *headers_end) {

    char *line = strstr(buf, "\r\n");   // 请求行的结尾, 指向 "\r\n" 的 '\r'

    while (line < headers_end) {

        size_t length;

        line += 2;  // 此行的起点

        /** 检查是否以 "Content-Length: " 开头 */
        if (sscanf(line, "Content-Length: %zu", &length) == 1) {
            return length;  // 如果满足则匹配字段值 %zu -> size_t, 赋值给 length
        }

        line = strstr(line, "\r\n");    // 此行的结尾
    }

    return 0;
}

size_t recv_http1_request(int sockfd, char *buf, size_t capacity) {

    size_t received = 0;        // 用于记录已经接收的字节长
    size_t request_length = 0;  // 用于记录 HTTP 请求报文总长

    while (1) {

        /** 接收数据, 返回此次接收 bytes 的长度 */
        received += (size_t)recv(
            sockfd,
            buf + received,             // 指定本次接收的 bytes 存于 buffer 时的起点
            capacity - 1 - received,    // 指定本次接收的最大字节数, 留 1 位用于字符串结尾 '\0'
            0                           // flags, 无特殊处理
        );

        buf[received] = '\0';   // 转成字符串, 循环继续时新数据会将 '\0' 覆盖

        /** 还未解析 headers */
        if (request_length == 0) {

            /** 查找请求头和请求体之间的分隔符, 指向 "\r\n\r\n" 的首个 '\r' */
            char *headers_end = strstr(buf, "\r\n\r\n");
            if (headers_end == NULL) {
                continue;
            }

            size_t headers_length = (size_t)(headers_end - buf) + 4; // [buf, ..., headers_end, "\r\n\r\n"]

            /** 获取 Content-Length 字段值, 它标记了请求体的字节长 */
            size_t body_length = get_content_length(buf, headers_end);

            request_length = headers_length + body_length;
        }

        if (received >= request_length) {
            return request_length;  // 已接收的字节长已经包含 HTTP 请求报文总长, 则返回
        }
    }
}
