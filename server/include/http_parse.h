#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include <stddef.h>

struct http_header {
    char *name;     // 字段名
    char *value;    // 字段值
};

struct http_request {
    char *method;   // 请求方法
    char *target;   // 资源路径
    char *version;  // HTTP 版本

    struct http_header headers[32]; // 请求头键值对(key:value)数组
    size_t header_count;            // 已存储请求头数

    char *body;         // 指向请求体起始位置
    size_t body_length; // 请求体长
};

/**
 * @brief 解析 HTTP 请求报文
 *
 * @param buf 指向 HTTP 请求报文起始位置的指针
 * @param length HTTP 请求报文总长
 * @param request 记录解析结果
 */
void parse_http1_request(char *buf, size_t length, struct http_request *request);

#endif
