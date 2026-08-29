#include "http_parse.h"

#include <string.h>


void parse_http1_request(char *buf, size_t length, struct http_request *request) {

    char *line_end = strstr(buf, "\r\n");           // 请求行结尾
    char *headers_end = strstr(buf, "\r\n\r\n");    // 请求头结尾

    /** method SPACE target SPACE version CRLF */
    char *method_end = strchr(buf, ' ');    // 第 1 个空格, 解析 method
    char *target = method_end + 1;          // 移动到空格后 1 位
    char *target_end = strchr(target, ' '); // 第 2 个空格, 解析 target
    char *version = target_end + 1;         // 移动到空格后 1 位, 解析 version

    /** 把空格与换行替换为 '\0', 分段字符串操作 */
    *method_end = '\0';
    *target_end = '\0';
    *line_end = '\0';

    /** 记录请求行元数据 */
    request->method = buf;
    request->target = target;
    request->version = version;
    
    request->header_count = 0;  // 请求头字段计数
    char *line = line_end + 2;  // 移动到请求头起始位置

    /** 解析请求头 */
    while (line < headers_end) {

        char *line_end = strstr(line, "\r\n");  // 这一行请求头的结尾
        char *colon = strchr(line, ':');        // 键值对(key:value)冒号分隔符

        *line_end = '\0';
        *colon = '\0';

        char *value = colon + 1;                    // 记录字段值的位置
        while (*value == ' ' || *value == '\t') {   // 跳过空格或制表符
            value++;
        }

        /** 记录请求头 */
        request->headers[request->header_count].name = line;
        request->headers[request->header_count].value = value;
        request->header_count++;

        line = line_end + 2;    // 移动到下一行请求头的起始位
    }

    /** 记录请求体 */
    request->body = headers_end + 4;
    request->body_length = length - (size_t)(request->body - buf);
}
