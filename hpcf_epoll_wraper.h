#ifndef HPCF_EPOLL_WRAPER
#define HPCF_EPOLL_WRAPER

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "hpcf_event.h"


void hpcf_epoll_init_event(struct hpcf_event *hev, int fd, int events, void *arg, hpcf_event_callback_t callback);

void hpcf_epoll_add_event(int epfd, struct hpcf_event *hev, int events);

void hpcf_epoll_del_event(int epfd, struct hpcf_event *hev);


#endif /* HPCF_EPOLL_WRAPER */
