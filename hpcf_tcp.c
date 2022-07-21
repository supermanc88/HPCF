#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>

#include "hpcf_tcp.h"
#include "hpcf_connection.h"
#include "hpcf_epoll_wraper.h"

extern int g_epoll_fd;
extern struct list_head g_other_task_queue;

// 用来建立连接的回调函数
void hpcf_tcp_accept_event_callback(int fd, int events, void *arg)
{
    // 开始创建连接，并申请一个hpcf_conncetion结构体，并将其加入到连接队列中
    struct hpcf_event *hev = (struct hpcf_event *)arg;

    // start accept
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int connfd = accept(fd, (struct sockaddr *)&addr, &addrlen);

    // 创建连接后，将connfd放到hpcf_connect结构中
    struct hpcf_connection *c = hpcf_new_connection(connfd,
                                                hpcf_tcp_read_event_callback,
                                                hpcf_tcp_write_event_callback,
                                                0);
    
    // 添加到epoll中
    hpcf_epoll_add_event(g_epoll_fd, c, EPOLLIN | EPOLLOUT);

}

// 读取数据的回调函数
void hpcf_tcp_read_event_callback(int fd, int events, void *arg)
{
    struct hpcf_event *hev = (struct hpcf_event *)arg;
    struct hpcf_connection *c = (struct hpcf_connection *)hev->data;

    // 如果hev中的ready为1，说明真的有数据进来了，开始读取数据
    if (hev->ready) {
        char *buf = c->buffer;
        int n = read(hev->fd, buf, MAX_BUF_SIZE);

        hev->ready = 0;

        // 如果读取的数据长度为0，说明连接已经断开了
        if (n == 0) {
            // 从epoll中删除连接
            hpcf_epoll_del_event(g_epoll_fd, c);
            // 关闭连接并释放连接所用内存
            hpcf_free_connection(c);
        } else if (n < 0) {
            // 从epoll中删除连接
            hpcf_epoll_del_event(g_epoll_fd, c);
            // 数据读取失败了，关闭连接并释放连接所用内存
            hpcf_free_connection(c);
        } else {
            // 读取成功后，将处理数据的任务放到任务队列中
            struct hpcf_event *proc_ev = hpcf_new_event(c->fd,
                                                        0,
                                                        hpcf_tcp_process_event_callback,
                                                        0,
                                                        c);
            // 添加到其它任务队列中
            list_add_tail(&proc_ev->list, &g_other_task_queue);
            // 表示数据已经拿到，处理数据回调可以激活了
            proc_ev->active = 1;
            hev->completed = 1;
            // hpcf_epoll_del_event(g_epoll_fd, c);
            // hpcf_epoll_add_event(g_epoll_fd, c, EPOLLOUT | EPOLLET);
        }
    }
}

// 处理数据的回调函数
void hpcf_tcp_process_event_callback(int fd, int events, void *arg)
{
    struct hpcf_event *hev = (struct hpcf_event *)arg;
    struct hpcf_connection *c = (struct hpcf_connection *)hev->data;
    struct hpcf_event *write_ev = c->write_event;

    char *buf = c->buffer;
    int n = strlen(buf);
    printf("recv: %s\n", buf);
    // 将数据处理完毕后，将数据处理完毕的标志置为1
    hev->completed = 1;
    write_ev->active = 1;

    // 释放hev内存
    hpcf_free_event(hev);
}

// 写入数据的回调函数
void hpcf_tcp_write_event_callback(int fd, int events, void *arg)
{
    struct hpcf_event *hev = (struct hpcf_event *)arg;
    struct hpcf_connection *c = (struct hpcf_connection *)hev->data;
    struct hpcf_event *read_ev = c->read_event;
    char *buf = c->buffer;

    // 开始发送数据
    int n = write(hev->fd, buf, strlen(buf));
    if (n <= 0) {
        printf("write error, errno = %d, %s\n", errno, strerror(errno));
        // 从epoll中删除连接
        hpcf_epoll_del_event(g_epoll_fd, c);
        // 发送失败，关闭连接并释放连接所用内存
        hpcf_free_connection(c);
    } else {
        // 发送成功，将数据处理完毕的标志置为1
        printf("send: %s\n", buf);
        read_ev->ready = 0;
        read_ev->completed = 0;
        hev->completed = 1;
        hev->active = 0;
    }
}