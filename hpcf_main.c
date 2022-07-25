#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#include "hpcf_main.h"
#include "hpcf_event.h"
#include "hpcf_fd_helper.h"
#include "hpcf_list_helper.h"
#include "hpcf_connection.h"
#include "hpcf_tcp.h"
#include "hpcf_epoll_wraper.h"
#include "hpcf_setaffinity.h"
#include "hpcf_module.h"

// 用来保存listenfd的事件 弃用
struct hpcf_event g_listen_event;
// 用来保存connectfd的事件 弃用
struct hpcf_event g_accept_events[HPCF_MAX_EVENTS];

// 用来监听listenfd的epollfd 弃用
int g_listen_epfd;
// 用来监听connectfd的epollfd 弃用
int g_accept_epfd;

// 用来监听listenfd和connectfd的epollfd
int g_epoll_fd;

// 用来做互斥锁的文件句柄
int g_lock_fd;

// 用来保存accept任务的队列
struct list_head g_accept_task_queue;
// 其它任务队列
struct list_head g_other_task_queue;

// 是否启用accept
int g_accept_disabled = 0;
// 空闲的connectfd数量
int g_free_connect_fd_num = HPCF_MAX_EVENTS;

// 用来保存所有已连接的connection队列
struct list_head g_all_connection_queue;

int g_listen_fd = 0;
struct hpcf_connection *g_listen_conn = NULL;

// void hpcf_read_data(int fd, void *arg)
// {
//     struct hpcf_other_task *task = (struct hpcf_other_task *)arg;
//     printf("%s process:%d read data from fd: %d\n", __func__, getpid(), fd);
//     // read to task->data
//     char *buf = task->data;
//     task->data_len = read(fd, buf, MAX_BUF_SIZE);
//     buf[task->data_len] = '\0';

//     if (task->data_len == 0) {
//         // 对方关闭了连接
//         printf("peer close connection\n");
//         task->step = HPCF_OTHER_TASK_STEP_CLOSE;
//         task->hpcf_event->conn_closed = 1;
//     } else if (task->data_len < 0) {
//         // 读取失败
//         printf("read data failed\n");
//         task->step = HPCF_OTHER_TASK_STEP_CLOSE;
//     } else {
//         // 读取成功
//         printf("read data: %s\n", buf);
//     }
// }

// void hpcf_process_data(int fd, void *arg)
// {
//     printf("%s process:%d process data from fd: %d\n", __func__, getpid(), fd);
//     struct hpcf_other_task *task = (struct hpcf_other_task *)arg;

//     // 处理完成后，将下一步改成写数据
//     task->step = OTHER_TASK_STEP_WRITE;

//     // 当数据处理完毕后，就要准备将数据发送给对方
//     // 需要在epoll中增加写事件
//     struct hpcf_event *ev = task->hpcf_event;
//     hpcf_epoll_del_event(g_accept_epfd, ev);
//     hpcf_epoll_init_event(ev, fd, EPOLLOUT, ev, hpcf_write_event_callback);
//     hpcf_epoll_add_event(g_accept_epfd, ev, EPOLLOUT);
// }

// void hpcf_write_data(int fd, void *arg)
// {
//     struct hpcf_other_task *task = (struct hpcf_other_task *)arg;
//     if (task->hpcf_event->conn_closed) {
//         task->step = HPCF_OTHER_TASK_STEP_CLOSE;
//         return;
//     }
//     if(task->hpcf_event->writable) {
//         task->step++;
//         // printf("%s\n", __func__);
//         // task->data_len = write(fd, task->data, task->data_len);
//         // printf("process:%d write %d data to fd: %d\n", getpid(), task->data_len, fd);
//         // if (task->data_len == -1) {
//         //     printf("write error: %s\n", strerror(errno));
//         //     // if (errno == EAGAIN) {
//         //     //     // 写缓冲区已满，需要等待再次写
//         //     //     task->step = OTHER_TASK_STEP_WRITE;
//         //     // } else {
//         //         // 其他错误
//         //         task->step = HPCF_OTHER_TASK_STEP_CLOSE;
//         //     // }
//         // } else if (task->data_len == 0) {
//         //     // 对方关闭了连接
//         //     task->step = HPCF_OTHER_TASK_STEP_CLOSE;
//         // } else {
//         //     // 写完了，继续监听读
//         //     task->step = OTHER_TASK_STEP_FINISH;
//         //     struct hpcf_event *ev = task->hpcf_event;
//         //     hpcf_epoll_del_event(g_accept_epfd, ev);
//         //     hpcf_epoll_init_event(ev, fd, EPOLLIN, ev, hpcf_read_event_callback);
//         //     hpcf_epoll_add_event(g_accept_epfd, ev, EPOLLIN);
//         // }
//     }
// }

// void hpcf_read_event_callback(int client_fd, int events, void *arg)
// {
//     // 添加任务到其它任务队列
//     printf("%s\n", __func__);
//     struct hpcf_event *hev = (struct hpcf_event *)arg;
//     struct hpcf_other_task *task = hpcf_new_other_task(client_fd,
//         hev, hpcf_read_data, hpcf_process_data, hpcf_write_data);

//     list_add_tail(&task->list, &g_other_task_queue);
// }

// void hpcf_write_event_callback(int client_fd, int events, void *arg)
// {
//     // 如果进入此流程，就说明连接可写，那么就标记cfd可写
//     printf("%s fd %d\n", __func__, client_fd);
//     struct hpcf_event *hev = (struct hpcf_event *)arg;
//     hev->writable = 1;
// }

// void hpcf_listen_callback(int listen_fd, int events, void *arg)
// {
//     // accept socket
//     struct sockaddr_in client_addr;
//     socklen_t client_addr_len = sizeof(client_addr);
//     int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
//     if (client_fd == -1) {
//         perror("accept error");
//         exit(1);
//     }
//     printf("accept a client, fd: %d\n", client_fd);
//     printf("client ip: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

//     // set nonblock
//     hpcf_set_fd_nonblock(client_fd);

//     // find a empty event and add it to epoll
//     int i;
//     do {
//         for (i = 0; i < HPCF_MAX_EVENTS; i++) {
//             if (g_accept_events[i].fd == -1) {
//                 break;
//             }
//         }
//         if (i == HPCF_MAX_EVENTS) {
//             printf("accept event is full\n");
//             close(client_fd);
//             return;
//         }
//         g_free_connect_fd_num--;
//         hpcf_epoll_init_event(&g_accept_events[i], client_fd, EPOLLIN, &g_accept_events[i], hpcf_read_event_callback);
//         hpcf_epoll_add_event(g_accept_epfd, &g_accept_events[i], EPOLLIN);
//     } while (0);
// }

// worker process function
void hpcf_worker_process_content()
{
    // 是否获取了互斥锁
    int is_get_lock = 0;

    // returned epoll event
    struct epoll_event revents[HPCF_MAX_EVENTS];

    // epoll wait timeout
    int timeout = 0;

    // init conn queue
    INIT_LIST_HEAD(&g_all_connection_queue);
    // init other task queue
    INIT_LIST_HEAD(&g_other_task_queue);

    // 这里是一个大循环
    for ( ; ; ) {
        // 1. 判断是否获取了互斥锁
        // 2. 如果获取了锁，才会去epoll_wait listen_epfd，才会去创建连接
        // 3. 否则去执行其它任务
        timeout = 0;

        do {
            if (g_accept_disabled > 0) {
                break;
            }

            if (hpcf_try_lock_fd(g_lock_fd) == 0) {
                is_get_lock = 1;
                // printf("%s %d get lock\n", __func__, getpid());
                hpcf_epoll_enable_accept_event(g_epoll_fd, g_listen_conn);
            } else {
                is_get_lock = 0;
                // break;
                hpcf_epoll_disable_accept_event(g_epoll_fd, g_listen_conn);
            }

            // 如果其它任务队列中有任务，则监听listen_epfd时不阻塞
            // 否则监听listen_epfd时阻塞
            if (list_empty(&g_other_task_queue)) {
                timeout = 1;
            } else {
                timeout = 0;
            }

            int n = epoll_wait(g_epoll_fd, revents, HPCF_MAX_EVENTS, timeout);
            int i;
            for (i = 0; i < n; i++) {
                printf("n = %d\n", n);
                struct epoll_event *event = &revents[i];
                // struct hpcf_event *hev = event->data.ptr;
                // if ( (event->events & EPOLLIN) && (hev->events & EPOLLIN) ) {
                //     hev->callback(hev->fd, hev->events, hev->arg);
                // }
                struct hpcf_connection *c = event->data.ptr;
                struct hpcf_event *rev = c->read_event;
                struct hpcf_event *wev = c->write_event;
                // printf("%s %d\n", __func__, c->fd);
                // printf("returned events = %d\n", event->events);
                if ( (event->events & EPOLLIN) && (rev->events & EPOLLIN) ) {
                    rev->active = 1;
                    rev->ready = 1;
                    rev->callback(rev->fd, rev->events, rev->arg);
                }

                if ( (event->events & EPOLLOUT) && wev->active ) {
                    wev->callback(wev->fd, wev->events, wev->arg);
                }
            }

            // 如果获取了锁，则释放
            if (is_get_lock) {
                hpcf_unlock_fd(g_lock_fd);
            }
        } while (0);

        // ==============================================================================
        // 上面处理了连接事件，下面处理其它任务
        // 其它任务也就是读写socket事件
        
        // 1. 遍历其它任务队列并处理
        hpcf_process_task_queue(&g_other_task_queue);
#if 0
        if (!list_empty(&g_other_task_queue)) {
            struct list_head *pos, *n;
            list_for_each_safe(pos, n, &g_other_task_queue) {
                struct hpcf_other_task *task = list_entry(pos, struct hpcf_other_task, list);
                printf("pid: %d fd: %d, step: %d\n", getpid(), task->fd, task->step);
                switch (task->step) {
                    case OTHER_TASK_STEP_INIT:
                    {
                        task->step++;
                    }
                    case OTHER_TASK_STEP_READ:
                    {
                        task->read_data(task->fd, task);
                        task->step++;
                    }
                    break;
                    case OTHER_TASK_STEP_PROCESS:
                    {
                        task->process_data(task->fd, task);
                        // task->step++;
                    }
                    break;
                    case OTHER_TASK_STEP_WRITE:
                    {
                        // TODO 这里很有可能写缓冲被占满了，需要后续处理，write的时候返回-1
                        task->write_data(task->fd, task);
                        // task->step++;
                    }
                    break;
                    case OTHER_TASK_STEP_FINISH:
                    {
                        // 一个tps完成
                        // 删除此任务
                        printf("%s %d finish %d task\n", __func__, getpid(), task->fd);
                        list_del(pos);
                    }
                    break;
                    default:
                    {
                        // 删除任务
                        list_del(pos);

                        // 从epoll中删除该fd，不再监听
                        hpcf_epoll_del_event(g_accept_epfd, task->hpcf_event);

                        hpcf_event_clear_1(task->hpcf_event);

                        g_free_connect_fd_num++;

                        g_accept_disabled = HPCF_MAX_EVENTS / 8 - g_free_connect_fd_num;
                    }
                    break;
                }
            }
        }
#endif

#if 0
        if (is_get_lock == 1) {
            // 说明上面抢到锁了，且无其它任务，当无连接进来时，上面休眠1ms，下面就不需要休眠了
            // 如果有其它任务时，上面没有超时，就尽快把任务执行完毕，下面也不需要休眠了
            timeout = 0;
        } else if (is_get_lock == 0) {
            timeout = 1;
        }
        int n = epoll_wait(g_accept_epfd, revents, HPCF_MAX_EVENTS, timeout);
        int i;
        for (i = 0; i < n; i++) {
            struct epoll_event *event = &revents[i];
            struct hpcf_event *hev = event->data.ptr;
            if ( (event->events & EPOLLIN) && (hev->events & EPOLLIN) ) {
                hev->callback(hev->fd, hev->events, hev->arg);
            }
            if ( (event->events & EPOLLOUT) && (hev->events & EPOLLOUT) ) {
                hev->callback(hev->fd, hev->events, hev->arg);
            }
        }
#endif
    }

}

// init listen socket
int hpcf_init_listen_socket(int listen_epfd, int listen_port)
{
    // create listen socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket error");
        exit(1);
    }

    // set socket option
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // bind socket
    struct sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(listen_port);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1) {
        perror("bind error");
        exit(1);
    }

    // listen socket
    if (listen(listen_fd, HPCF_LISTEN_BACKLOG) == -1) {
        perror("listen error");
        exit(1);
    }

    int s_rec = 0;
    int s_send = 0;

    // 获取socket的发送和接收缓冲区大小
    socklen_t len = sizeof(s_rec);
    getsockopt(listen_fd, SOL_SOCKET, SO_RCVBUF, &s_rec, &len);
    getsockopt(listen_fd, SOL_SOCKET, SO_SNDBUF, &s_send, &len);
    printf("listen socket recv buf size: %d, send buf size: %d\n", s_rec, s_send);

    // 设置socket的发送和接收缓冲区大小
    s_rec = 1024 * 1024 * 1024;
    s_send = 1024 * 1024 * 1024;
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVBUF, &s_rec, sizeof(s_rec));
    // setsockopt(listen_fd, SOL_SOCKET, SO_SNDBUF, &s_send, sizeof(s_send));

    // set listen socket nonblock
    hpcf_set_fd_nonblock(listen_fd);

    // init listen socket event
    // hpcf_epoll_init_event(&g_listen_event, listen_fd, EPOLLIN, &g_listen_epfd, hpcf_listen_callback);
    // hpcf_epoll_add_event(listen_epfd, &g_listen_event, EPOLLIN);

    // // 创建一个用来保存listenfd的hpcf_connection
    // struct hpcf_connection *listen_conn = hpcf_new_connection(listen_fd,
    //                                             hpcf_tcp_accept_event_callback,
    //                                             NULL,
    //                                             1);

    // // add listen_fd to epoll
    // hpcf_epoll_add_event(g_epoll_fd, listen_conn, EPOLLIN);

    return listen_fd;

}

int main(int argc, char *argv[])
{
    int ret = 0;

    // 初始化存放listenfd和connectfd事件的结构体
    // hpcf_event_clear_1(&g_listen_event);
    // hpcf_event_clear_n(g_accept_events, HPCF_MAX_EVENTS);

#if 0
    g_listen_epfd = epoll_create(1);
    if (g_listen_epfd == -1) {
        perror("epoll_create error");
        exit(1);
    }
    g_accept_epfd = epoll_create(HPCF_MAX_EVENTS);
    if (g_accept_epfd == -1) {
        perror("epoll_create error");
        exit(1);
    }
#endif

    // 打开一个用来作互斥锁的文件
    g_lock_fd = open("/tmp/lock.file", O_RDWR | O_CREAT, 0666);


    int listen_fd = hpcf_init_listen_socket(g_epoll_fd, HPCF_LISTEN_PORT);

    // create worker process
    int i;
    for (i = 0; i < HPCF_WORKER_PROCESS_NUM; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // worker process
            // set worker process affinity
            hpcf_set_affinity(i);
            // 创建epollfd
            g_epoll_fd = epoll_create(HPCF_MAX_EVENTS);
            struct hpcf_connection *listen_conn = hpcf_new_connection(listen_fd,
                                            hpcf_tcp_accept_event_callback,
                                            NULL,
                                            1);
            
            g_listen_fd = listen_fd;
            g_listen_conn = listen_conn;

            printf("g_listen_fd: %d g_listen_conn->fd: %d\n", g_listen_fd, g_listen_conn->fd);

            // add listen_fd to epoll
            hpcf_epoll_add_event(g_epoll_fd, listen_conn, EPOLLIN);

            // module init
            hpcf_module_manager_init();
            // register module
            hpcf_register_processor_modules(NULL);

            hpcf_worker_process_content();
            break;
        } else if (pid > 0) {
            // parent process
        } else {
            perror("fork error");
            exit(1);
        }
    }

    // wait for worker process
    wait(NULL);

    return ret;
}