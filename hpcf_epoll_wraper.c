#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "hpcf_connection.h"
#include "hpcf_epoll_wraper.h"

extern int g_listen_fd;    // 监听的fd
extern struct hpcf_connection *g_listen_conn; // 监听的connection


void hpcf_epoll_init_event(struct hpcf_event *hev, int fd, int events, void *arg, hpcf_event_callback_t callback)
{
    // hev->fd = fd;
    // hev->events = events | EPOLLET;
    // hev->arg = arg;
    // hev->writable = 0;
    // hev->conn_closed = 0;
    // hev->callback = callback;
    // hev->active = 0;
}

void hpcf_epoll_add_event(int epfd, struct hpcf_connection *conn, int events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = conn;
    struct hpcf_event *rev = conn->read_event;
    struct hpcf_event *wev = conn->write_event;

    int opt;
    if (rev->active || wev->active) {
        opt = EPOLL_CTL_MOD;
    } else {
        opt = EPOLL_CTL_ADD;
    }

    if (epoll_ctl(epfd, opt, conn->fd, &ev) == -1) {
        perror("epoll_ctl add error");
        exit(1);
    } else {
        conn->in_epoll = 1;
        printf("epoll_ctl EPOLL_CTL_ADD, fd: %d, events: %d\n", conn->fd, events);
    }
}

void hpcf_epoll_del_event(int epfd, struct hpcf_connection *conn)
{
    // if (!hev->active) {
    //     return;
    // }

    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = NULL;
    printf("%s epoll_ctl del fd: %d\n", __func__, conn->fd);
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, &ev) == -1) {
        perror("epoll_ctl del error");
        exit(1);
    } else {
        conn->in_epoll = 0;
        printf("epoll_ctl opt: EPOLL_CTL_DEL, fd: %d\n", conn->fd);
    }
}

void hpcf_epoll_enable_accept_event(int epfd, struct hpcf_connection *conn)
{
    if (conn->in_epoll) {
        return;
    }
    // hpcf_epoll_add_event(epfd, conn, EPOLLIN);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = conn;
    epoll_ctl(epfd, EPOLL_CTL_ADD, conn->fd, &ev);
    conn->in_epoll = 1;
    // printf("%s epoll_ctl EPOLLIN fd: %d\n", __func__, conn->fd);
}

void hpcf_epoll_disable_accept_event(int epfd, struct hpcf_connection *conn)
{
    if (!conn->in_epoll) {
        return;
    }
    // hpcf_epoll_del_event(epfd, conn);
    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = NULL;
    epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, &ev);
    conn->in_epoll = 0;
    // printf("%s epoll_ctl EPOLL_CTL_DEL fd: %d\n", __func__, conn->fd);
}