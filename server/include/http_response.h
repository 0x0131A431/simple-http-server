#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/**
 * @brief 向客户端发送数据
 *
 * @param sockfd socket 文件描述符
 */
void send_http1_response(int sockfd);

#endif
