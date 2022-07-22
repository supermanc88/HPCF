#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "hpcf_connection.h"
#include "hpcf_epoll_wraper.h"


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
        // hev->active = 1;
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
        // hev->active = 0;
        printf("epoll_ctl opt: EPOLL_CTL_DEL, fd: %d\n", conn->fd);
    }
}