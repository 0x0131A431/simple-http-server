#include "http_response.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>


/**
 * @brief 发送目标全部数据
 *
 * @param sockfd socket 文件描述符
 * @param data 目标响应数据, 只读
 * @param length 目标响应数据长度
 */
static void send_all(int sockfd, const void *data, size_t length) {

    const char *bytes = data;   // 准备发送的目标数据
    size_t sent = 0;            // 记录已发送字节数

    while (sent < length) {

        /** 发送数据, 返回此次发送 bytes 的长度 */
        ssize_t count = send(
            sockfd,
            bytes + sent,       // 指定本次发送的 bytes 起点
            length - sent,      // 指定本次发送的最大字节数
            0                   // flags, 无特殊处理
        );

        /** 什么都没发就返回 */
        if (count <= 0) {
            return;
        }

        sent += (size_t)count;
    }
}

void send_http1_response(int sockfd, struct http_request *request) {

    int status_code;            // 状态码
    const char *reason;         // 原因短语
    const char *body;           // 响应体指针
    size_t body_length;         // 响应体长度
    const char *allow = NULL;   // Allow 头字段

    /** 固定响应体 */
    static const char hello_body[] = "Hello World!";
    static const char not_found_body[] = "Not Found";
    static const char method_not_allowed_body[] = "Method Not Allowed";

    /** 判断请求路径 */
    int is_hello_path =
        strcmp(request->path, "/") == 0 ||
        strcmp(request->path, "/hello") == 0;
    int is_echo_path =
        strcmp(request->path, "/echo") == 0;

    if (is_hello_path) {
        /** / 和 /hello 支持 GET, HEAD */
        if (
            strcmp(request->method, "GET") == 0 ||
            strcmp(request->method, "HEAD") == 0
        ) {
            status_code = 200;
            reason = "OK";
            body = hello_body;
            body_length = sizeof hello_body - 1;
        }
        else {
            status_code = 405;
            reason = "Method Not Allowed";
            body = method_not_allowed_body;
            body_length = sizeof method_not_allowed_body - 1;
            allow = "GET, HEAD";
        }
    }
    else if (is_echo_path) {
        /** /echo 支持 POST */
        if (strcmp(request->method, "POST") == 0) {
            status_code = 200;
            reason = "OK";
            body = request->body;
            body_length = request->body_length;
        }
        else {
            status_code = 405;
            reason = "Method Not Allowed";
            body = method_not_allowed_body;
            body_length = sizeof method_not_allowed_body - 1;
            allow = "POST";
        }
    }
    else {
        status_code = 404;
        reason = "Not Found";
        body = not_found_body;
        body_length = sizeof not_found_body - 1;
    }

    char response_headers[256]; // 用于写入字符串的缓冲区
    int response_headers_length;// 缓冲区大小

    /** 拼响应头 */
    if (allow != NULL) {
        /** 格式化字符串, 返回本该写入缓冲区的长度 */
        response_headers_length = snprintf(
            response_headers,
            sizeof response_headers,
            "HTTP/1.1 %d %s\r\n"
            "Allow: %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",     // format, 顺序填入后续可选参数
            status_code,
            reason,
            allow,
            body_length
        );
    }
    else {
        response_headers_length = snprintf(
            response_headers,
            sizeof response_headers,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            status_code,
            reason,
            body_length
        );
    }

    /** 格式化写入失败返回 < 0 */
    if (response_headers_length < 0) {
        return;
    }

    send_all(
        sockfd,
        response_headers,
        (size_t)response_headers_length
    );

    /** 不是 HEAD 请求继续发送 body */
    if (strcmp(request->method, "HEAD") != 0) {
        send_all(sockfd, body, body_length);
    }
}
