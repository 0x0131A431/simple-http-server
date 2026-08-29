#ifndef HTTP_RECV_H
#define HTTP_RECV_H

#include <stddef.h>

/**
 * @brief 用于从字节流中获取完整 HTTP 请求报文 bytes
 *
 * @param sockfd socket 文件描述符
 * @param buf 可写的 buffer
 * @param capacity buffer 容量
 * @return 返回 HTTP 请求报文总长度
 */
size_t recv_http1_request(int sockfd, char *buf, size_t capacity);

#endif
