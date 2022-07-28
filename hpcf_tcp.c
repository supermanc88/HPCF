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
#include "hpcf_module.h"
#include "hpcf_json_helper.h"
#include "hpcf_module_type.h"

extern int g_epoll_fd;
extern struct list_head g_other_task_queue;

// 解析tcp请求中的数据
int hpcf_tcp_parse_data_from_header(char *json, struct hpcf_tcp_data_header *header)
{
    int ret = 0;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Object *header_object = NULL;

    memset(header, 0, sizeof(struct hpcf_tcp_data_header));

    root_value = json_parse_string(json);
    if (root_value == NULL) {
        printf("json_parse_string error\n");
        ret = -1;
        goto out;
    }
    root_object = json_value_get_object(root_value);
    if (root_object == NULL) {
        printf("json_value_get_object error\n");
        ret = -1;
        goto out;
    }
    header_object = json_object_get_object(root_object, "Header");
    if (header_object == NULL) {
        printf("json_object_get_object error\n");
        ret = -1;
        goto out;
    }
    int version = json_object_get_number(header_object, "Version");
    header->version = version;
    char *s = json_object_get_string(header_object, "User");
    memcpy(header->user, s, strlen(s));
    s = json_object_get_string(header_object, "Announce");
    memcpy(header->announce, s, strlen(s));
    s = json_object_get_string(header_object, "Sessionid");
    if (strlen(s) != 0 || s != NULL)
        memcpy(header->session_id, s, strlen(s));
    s = json_object_get_string(header_object, "RequestType");
    memcpy(header->request_type, s, strlen(s));

out:
    return ret;
}

// 这里通过读到的数据，进行分析，
// 根据type，从模块队列中找到对应的模块，然后调用模块的回调函数
void hpcf_tcp_process_read_data(struct hpcf_connection *conn)
{
    int ret = 0;
    int type = HPCF_MODULE_TYPE_UNKNOWN;
    struct hpcf_tcp_data_header header = {0};

    ret = hpcf_tcp_parse_data_from_header(conn->read_buffer, &header);
    if (ret == 0) {
        printf("hpcf_tcp_parse_data_from_header error\n");
        if (strcmp(header.request_type, "Session") == 0) {
            type = HPCF_MODULE_TYPE_LOGIN_AUTH;
        }
        // 下面有很多else if，通过比较设置type
        // else if
    }

    // 假如这个解析到的是0
    struct hpcf_processor_module *module = NULL;
    module = hpcf_get_processor_module_by_type(type);
    if (module == NULL) {
        // 如果找不到对应的模块，就把接收到的数据吐回去
        memcpy(conn->write_buffer, conn->read_buffer, conn->read_len);
        conn->write_len = conn->read_len;
        conn->write_buffer[conn->write_len] = '\0';
        printf("no module for type: %d\n", type);
        return;
    }
    hpcf_module_processor_callback callback = module->callback;
    // 这里处理完毕后，数据就写到了conn->write_buffer中，后面再写到socket中就可以了
    // module->data是模块数据存放指针
    // TODO: 目前感觉还需要再传递一个此连接存放数据的指针，用来存放当前连接的一些数据
    ret = callback(conn->read_buffer, conn->read_len, conn->write_buffer, &conn->write_len, &module->data, &conn->data);
}

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
       // 获取socket的发送和接收缓冲区大小
    int s_rec = 0;
    int s_send = 0;
    socklen_t len = sizeof(s_rec);
    getsockopt(connfd, SOL_SOCKET, SO_RCVBUF, &s_rec, &len);
    getsockopt(connfd, SOL_SOCKET, SO_SNDBUF, &s_send, &len);
    printf("listen socket recv buf size: %d, send buf size: %d\n", s_rec, s_send); 
    // 添加到epoll中
    hpcf_epoll_add_event(g_epoll_fd, c, EPOLLIN|EPOLLET|EPOLLRDHUP);

}

// 读取数据的回调函数
void hpcf_tcp_read_event_callback(int fd, int events, void *arg)
{
    struct hpcf_event *hev = (struct hpcf_event *)arg;
    struct hpcf_connection *c = (struct hpcf_connection *)hev->data;
    struct hpcf_event *wev = c->write_event;

    // 如果hev中的ready为1，说明真的有数据进来了，开始读取数据
    if (hev->ready) {
        char *buf = c->read_buffer;
        int n = read(hev->fd, buf, MAX_BUF_SIZE);

        hev->ready = 0;

        // 如果读取的数据长度为0，说明连接已经断开了
        if (n == 0) {
            printf("connection %d closed\n", hev->fd);
            // 从epoll中删除连接
            hpcf_epoll_del_event(g_epoll_fd, c);
            // 关闭连接并释放连接所用内存
            hpcf_free_connection(c);
            
            if (wev->active) {
                wev->active = 0;
            }
        } else if (n < 0) {
            // 从epoll中删除连接
            hpcf_epoll_del_event(g_epoll_fd, c);
            // 数据读取失败了，关闭连接并释放连接所用内存
            hpcf_free_connection(c);
        } else {
            c->read_len = n;
            buf[n] = '\0';
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

    char *buf = c->read_buffer;
    int n = strlen(buf);
    // printf("recv from %d: %s\n", hev->fd, buf);

    // 数据处理
    hpcf_tcp_process_read_data(c);
    // 模拟的数据处理
    // memcpy(c->write_buffer, buf, c->read_len);
    // c->write_len = c->read_len;

    // 将数据处理完毕后，将数据处理完毕的标志置为1
    hev->completed = 1;
    write_ev->active = 1;

    // 数据处理完毕后开始激活写事件
    hpcf_epoll_add_event(g_epoll_fd, c, EPOLLOUT);

    // 释放hev内存
    hpcf_free_event(hev);
}

// 写入数据的回调函数
void hpcf_tcp_write_event_callback(int fd, int events, void *arg)
{
    struct hpcf_event *wev = (struct hpcf_event *)arg;
    struct hpcf_connection *c = (struct hpcf_connection *)wev->data;
    struct hpcf_event *read_ev = c->read_event;
    char *buf = c->write_buffer;

    // 开始发送数据
    int n = write(wev->fd, buf, c->write_len);
    if (n <= 0) {
        printf("write error, fd = %d %s\n", wev->fd, strerror(errno));
        // 从epoll中删除连接
        hpcf_epoll_del_event(g_epoll_fd, c);
        // 发送失败，关闭连接并释放连接所用内存
        hpcf_free_connection(c);
    } else {
        // 发送成功，将数据处理完毕的标志置为1
        hpcf_epoll_add_event(g_epoll_fd, c, EPOLLIN |EPOLLET| EPOLLHUP);

        // printf("send to %d: %s\n", wev->fd, buf);
        read_ev->ready = 0;
        read_ev->active = 0;
        read_ev->completed = 0;
        wev->completed = 1;
        wev->active = 0;
    }
}