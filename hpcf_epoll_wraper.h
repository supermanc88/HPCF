#ifndef HPCF_EPOLL_WRAPER
#define HPCF_EPOLL_WRAPER

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "hpcf_event.h"


void hpcf_epoll_init_event(struct hpcf_event *hev, int fd, int events, void *arg, hpcf_event_callback_t callback);

void hpcf_epoll_add_event(int epfd, struct hpcf_connection *conn, int events);

void hpcf_epoll_del_event(int epfd, struct hpcf_connection *conn);

void hpcf_epoll_enable_accept_event(int epfd, struct hpcf_connection *conn);

void hpcf_epoll_disable_accept_event(int epfd, struct hpcf_connection *conn);


#endif /* HPCF_EPOLL_WRAPER */
