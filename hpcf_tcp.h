#ifndef HPCF_TCP
#define HPCF_TCP

// 用来建立连接的回调函数
void hpcf_tcp_accept_event_callback(int fd, int events, void *arg);

// 读取数据的回调函数
void hpcf_tcp_read_event_callback(int fd, int events, void *arg);

// 处理数据的回调函数
void hpcf_tcp_process_event_callback(int fd, int events, void *arg);

// 写入数据的回调函数
void hpcf_tcp_write_event_callback(int fd, int events, void *arg);

#endif /* HPCF_TCP */
