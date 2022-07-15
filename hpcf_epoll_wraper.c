#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "hpcf_epoll_wraper.h"


void hpcf_epoll_init_event(struct hpcf_event *hev, int fd, int events, void *arg, hpcf_event_callback_t callback)
{
    hev->fd = fd;
    hev->events = events;
    hev->arg = arg;
    hev->callback = callback;
    hev->active = 0;
}

void hpcf_epoll_add_event(int epfd, struct hpcf_event *hev, int events)
{
    struct epoll_event ev;
    hev->events = ev.events = events;
    ev.data.ptr = hev;

    int opt;
    if (hev->active) {
        opt = EPOLL_CTL_MOD;
    } else {
        opt = EPOLL_CTL_ADD;
    }

    if (epoll_ctl(epfd, opt, hev->fd, &ev) == -1) {
        perror("epoll_ctl error");
        exit(1);
    } else {
        hev->active = 1;
        printf("epoll_ctl opt: %d, fd: %d, events: %d\n", opt, hev->fd, events);
    }
}

void hpcf_epoll_del_event(int epfd, struct hpcf_event *hev)
{
    if (!hev->active) {
        return;
    }

    struct epoll_event ev;
    ev.data.ptr = hev;
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, hev->fd, &ev) == -1) {
        perror("epoll_ctl error");
        exit(1);
    } else {
        hev->active = 0;
        printf("epoll_ctl opt: EPOLL_CTL_DEL, fd: %d\n", hev->fd);
    }
}