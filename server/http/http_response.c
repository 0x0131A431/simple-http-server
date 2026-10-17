#include "http_response.h"

#include <fcntl.h>      // open, O_RDONLY
#include <unistd.h>     // close, read
#include <limits.h>     // PATH_MAX
#include <sys/stat.h>   // fstat, struct stat, S_ISREG
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

static const char method_not_allowed_body[] = "Method Not Allowed";

/**
 * @brief 组装并发送请求头
 *
 * @param sockfd socket 文件描述符
 * @param status_code 状态码
 * @param reason 结果描述
 * @param allow 允许的方法, 触发"Method not Allow"时不为空
 * @param content_type 声明响应体的类型
 * @param body_length 响应体长度
 */
static void send_response_headers(
    int sockfd,
    int status_code,
    const char *reason,
    const char *allow,
    const char *content_type,
    size_t body_length
) {
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
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",     // format, 顺序填入后续可选参数
            status_code,
            reason,
            allow,
            content_type,
            body_length
        );
    }
    else {
        response_headers_length = snprintf(
            response_headers,
            sizeof response_headers,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            status_code,
            reason,
            content_type,
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
}

/**
 * @brief 判断文件扩展名, 返回对应的 MIME 类型
 * 
 * @param path 文件路径
 * @return MIME 类型字符串
 */
static const char *get_content_type(const char *path) {
    /** 查找字符串最后一个点号 ., 返回指针指向点号 . */
    const char *extension = strrchr(path, '.');

    if (extension == NULL) {
        return "application/octet-stream";
    }

    if (strcmp(extension, ".html") == 0) {
        return "text/html";
    }

    if (strcmp(extension, ".css") == 0) {
        return "text/css";
    }

    if (strcmp(extension, ".js") == 0) {
        return "application/javascript";
    }

    if (strcmp(extension, ".txt") == 0) {
        return "text/plain";
    }

    return "application/octet-stream";
}

/**
 * @brief 根据 HTTP 请求中的 URL 路径, 拼接出服务器本地静态文件的路径
 * 
 * @param request_path HTTP 请求 URL 路径
 * @param file_path 用于存放拼接后文件路径的缓冲区
 * @param file_path_capacity 缓冲区的容量
 * @return 检查拼接结果是否能够完整放入目标缓冲区
 */
static int build_static_file_path(
    const char *request_path,
    char *file_path,
    size_t file_path_capacity
) {
    const char *resource_path = request_path;

    /** 如果请求根路径则指向 index.html */
    if (strcmp(request_path, "/") == 0) {
        resource_path = "/index.html";
    }

    int path_length = snprintf(
        file_path,
        file_path_capacity,
        "www%s",
        resource_path
    );

    return path_length >= 0 && (size_t)path_length < file_path_capacity;
}

/**
 * @brief 根据 HTTP 请求读取服务器静态文件, 并响应给客户端
 * @param sockfd socket 文件描述符
 * @param request HTTP 解析结果
 * @return 成功 0/1 状态
 */
static int send_static_file_response(
    int sockfd,
    struct http_request *request
) {
    char file_path[PATH_MAX];   // 存放文件路径的缓存区
    if (!build_static_file_path(request->path, file_path, sizeof file_path)) {
        return 0;
    }

    /** 按文件路径打开文件, 获得文件描述符 */
    int filefd = open(
        file_path,
        O_RDONLY    // 以只读方式打开
    );
    if (filefd < 0) {
        return 0;
    }

    /**
     * 检查文件是否为普通文件
     * - fstat 获取文件信息, 获取失败返回 -1
     * - S_ISREG 判断是不是普通文本文件
     */
    struct stat file_info;
    if (fstat(filefd, &file_info) < 0 || !S_ISREG(file_info.st_mode)) {
        close(filefd);
        return 0;
    }

    /** 仅支持 GET, HEAD */
    if (
        strcmp(request->method, "GET") != 0 &&
        strcmp(request->method, "HEAD") != 0
    ) {
        send_response_headers(
            sockfd,
            405,
            "Method Not Allowed",
            "GET, HEAD",
            "text/plain",
            sizeof method_not_allowed_body - 1
        );
        send_all(
            sockfd,
            method_not_allowed_body,
            sizeof method_not_allowed_body - 1
        );
        close(filefd);
        return 1;
    }

    send_response_headers(
        sockfd,
        200,
        "OK",
        NULL,
        get_content_type(file_path),
        (size_t)file_info.st_size
    );

    /** GET 发送文件内容 */
    if (strcmp(request->method, "GET") == 0) {
        char file_buffer[8192]; // 文件内容缓冲区
        ssize_t bytes_read;

        /** read() 循环读文件并发送, 每次最多 8192 Bytes */
        while ((bytes_read = read(filefd, file_buffer, sizeof file_buffer)) > 0) {
            send_all(sockfd, file_buffer, (size_t)bytes_read);
        }
    }

    close(filefd);
    return 1;
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

    /** 判断请求路径 */
    int is_hello_path = strcmp(request->path, "/hello") == 0;
    int is_echo_path = strcmp(request->path, "/echo") == 0;

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
    else if (send_static_file_response(sockfd, request)) {
        return;
    }
    else {
        status_code = 404;
        reason = "Not Found";
        body = not_found_body;
        body_length = sizeof not_found_body - 1;
    }

    /** 先发请求头 */
    send_response_headers(
        sockfd,
        status_code,
        reason,
        allow,
        "text/plain",
        body_length
    );

    /** 不是 HEAD 请求继续发送 body */
    if (strcmp(request->method, "HEAD") != 0) {
        send_all(sockfd, body, body_length);
    }
}
