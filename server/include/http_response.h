#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_parse.h"

/**
 * @brief 向客户端发送数据
 *
 * @param sockfd socket 文件描述符
 * @param request 接收 HTTP 解析结果
 */
void send_http1_response(int sockfd, struct http_request *request);

#endif
