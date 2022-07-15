#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#include "hpcf_main.h"
#include "hpcf_event.h"
#include "hpcf_epoll_wraper.h"
#include "hpcf_fd_helper.h"
#include "hpcf_list_helper.h"

// 用来保存listenfd的事件
struct hpcf_event g_listen_event;
// 用来保存connectfd的事件
struct hpcf_event g_accept_events[HPCF_MAX_EVENTS];

// 用来监听listenfd的epollfd
int g_listen_epfd;
// 用来监听connectfd的epollfd
int g_accept_epfd;

// 用来做互斥锁的文件句柄
int g_lock_fd;

// 其它任务队列
struct list_head g_other_task_queue;

// 是否启用accept
int g_accept_disabled = 0;
// 空闲的connectfd数量
int g_free_connect_fd_num = HPCF_MAX_EVENTS;

void hpcf_read_data(int fd, void *arg)
{
    struct hpcf_other_task *task = (struct hpcf_other_task *)arg;
    printf("process:%d read data from fd: %d\n", getpid(), fd);
    // read to task->data
    char *buf = task->data;
    task->data_len = read(fd, buf, MAX_BUF_SIZE);
    buf[task->data_len] = '\0';

    if (task->data_len == 0) {
        // 对方关闭了连接
        printf("peer close connection\n");
        task->step = HPCF_OTHER_TASK_STEP_CLOSE;
    }
}

void hpcf_process_data(int fd, void *arg)
{
    printf("process:%d process data from fd: %d\n", getpid(), fd);
    struct hpcf_other_task *task = (struct hpcf_other_task *)arg;
}

void hpcf_write_data(int fd, void *arg)
{
    struct hpcf_other_task *task = (struct hpcf_other_task *)arg;
    task->data_len = write(fd, task->data, task->data_len);
    printf("process:%d write %d data to fd: %d\n", getpid(), task->data_len, fd);

}

void hpcf_read_data_callback(int client_fd, int events, void *arg)
{
    // 添加任务到其它任务队列
    struct hpcf_event *hev = (struct hpcf_event *)arg;
    struct hpcf_other_task *task = hpcf_new_other_task(client_fd,
        hev, hpcf_read_data, hpcf_process_data, hpcf_write_data);

    list_add_tail(&task->list, &g_other_task_queue);
}

void hpcf_write_data_callback(int client_fd, int events, void *arg)
{

}

void hpcf_listen_callback(int listen_fd, int events, void *arg)
{
    // accept socket
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        perror("accept error");
        exit(1);
    }
    printf("accept a client, fd: %d\n", client_fd);
    printf("client ip: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // set nonblock
    hpcf_set_fd_nonblock(client_fd);

    // find a empty event and add it to epoll
    int i;
    do {
        for (i = 0; i < HPCF_MAX_EVENTS; i++) {
            if (g_accept_events[i].fd == -1) {
                break;
            }
        }
        if (i == HPCF_MAX_EVENTS) {
            printf("accept event is full\n");
            close(client_fd);
            return;
        }
        g_free_connect_fd_num--;
        hpcf_epoll_init_event(&g_accept_events[i], client_fd, EPOLLIN, &g_accept_events[i], hpcf_read_data_callback);
        hpcf_epoll_add_event(g_accept_epfd, &g_accept_events[i], EPOLLIN);
    } while (0);
}

// worker process function
void hpcf_worker_process_content()
{
    // 是否获取了互斥锁
    int is_get_lock = 0;

    // returned epoll event
    struct epoll_event revents[HPCF_MAX_EVENTS];

    // epoll wait timeout
    int timeout = 1;

    // init other task queue
    INIT_LIST_HEAD(&g_other_task_queue);

    // 这里是一个大循环
    for ( ; ; ) {
        // 1. 判断是否获取了互斥锁
        // 2. 如果获取了锁，才会去epoll_wait listen_epfd，才会去创建连接
        // 3. 否则去执行其它任务

        do {
            if (g_accept_disabled > 0) {
                break;
            }

            if (hpcf_try_lock_fd(g_lock_fd) == 0) {
                is_get_lock = 1;
            } else {
                break;
            }

            // 如果其它任务队列中有任务，则监听listen_epfd时不阻塞
            // 否则监听listen_epfd时阻塞
            if (list_empty(&g_other_task_queue)) {
                timeout = 1;
            } else {
                timeout = 0;
            }

            int n = epoll_wait(g_listen_epfd, revents, HPCF_MAX_EVENTS, timeout);
            int i;
            for (i = 0; i < n; i++) {
                struct epoll_event *event = &revents[i];
                struct hpcf_event *hev = event->data.ptr;
                if ( (event->events & EPOLLIN) && (hev->events & EPOLLIN) ) {
                    hev->callback(hev->fd, hev->events, hev->arg);
                }
            }

            // 如果获取了锁，则释放
            if (is_get_lock) {
                hpcf_unlock_fd(g_lock_fd);
                is_get_lock = 0;
            }
        } while (0);

        // ==============================================================================
        // 上面处理了连接事件，下面处理其它任务
        // 其它任务也就是读写socket事件
        
        // 1. 遍历其它任务队列并处理
        if (!list_empty(&g_other_task_queue)) {
            struct list_head *pos, *n;
            list_for_each_safe(pos, n, &g_other_task_queue) {
                struct hpcf_other_task *task = list_entry(pos, struct hpcf_other_task, list);
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
                        task->step++;
                    }
                    break;
                    case OTHER_TASK_STEP_WRITE:
                    {
                        task->write_data(task->fd, task);
                        task->step++;
                    }
                    break;
                    case OTHER_TASK_STEP_FINISH:
                    {
                        // 一个tps完成
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

        int n = epoll_wait(g_accept_epfd, revents, HPCF_MAX_EVENTS, timeout);
        int i;
        for (i = 0; i < n; i++) {
            struct epoll_event *event = &revents[i];
            struct hpcf_event *hev = event->data.ptr;
            if ( (event->events & EPOLLIN) && (hev->events & EPOLLIN) ) {
                hev->callback(hev->fd, hev->events, hev->arg);
            }
        }
    }

}

// init listen socket
void hpcf_init_listen_socket(int listen_epfd, int listen_port)
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

    // set listen socket nonblock
    hpcf_set_fd_nonblock(listen_fd);

    // init listen socket event
    hpcf_epoll_init_event(&g_listen_event, listen_fd, EPOLLIN, &g_listen_epfd, hpcf_listen_callback);
    hpcf_epoll_add_event(listen_epfd, &g_listen_event, EPOLLIN);

}

int main(int argc, char *argv[])
{
    int ret = 0;

    // 初始化存放listenfd和connectfd事件的结构体
    hpcf_event_clear_1(&g_listen_event);
    hpcf_event_clear_n(g_accept_events, HPCF_MAX_EVENTS);

    // 创建epollfd
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

    // 打开一个用来作互斥锁的文件
    g_lock_fd = open("/tmp/lock.file", O_RDWR | O_CREAT, 0666);

    hpcf_init_listen_socket(g_listen_epfd, HPCF_LISTEN_PORT);

    // create worker process
    int i;
    for (i = 0; i < HPCF_WORKER_PROCESS_NUM; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // worker process
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