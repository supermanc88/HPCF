#ifndef HPCF_TCP
#define HPCF_TCP

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "hpcf_connection.h"

// 框架只关心数据中的header，body直接向下发送
struct hpcf_tcp_data_header {
    int version;        // 版本
    char user[32];      // 用户名
    char announce[32];  // 挑战码
    char session_id[32]; // 会话ID
    char request_type[32]; // 请求类型
};

// 解析tcp请求中的数据
int hpcf_tcp_parse_data_from_header(char *json, struct hpcf_tcp_data_header *header);

// 初步处理接收的数据
void hpcf_tcp_process_read_data(struct hpcf_connection *conn);

// 用来建立连接的回调函数
void hpcf_tcp_accept_event_callback(int fd, int events, void *arg);

// 读取数据的回调函数
void hpcf_tcp_read_event_callback(int fd, int events, void *arg);

// 处理数据的回调函数
void hpcf_tcp_process_event_callback(int fd, int events, void *arg);

// 写入数据的回调函数
void hpcf_tcp_write_event_callback(int fd, int events, void *arg);

#endif /* HPCF_TCP */
