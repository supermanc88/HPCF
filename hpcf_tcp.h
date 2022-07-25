#ifndef HPCF_TCP
#define HPCF_TCP

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "hpcf_connection.h"

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
