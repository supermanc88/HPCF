#ifndef HPCF_EVENT
#define HPCF_EVENT

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "hpcf_list_helper.h"

typedef void (*hpcf_event_callback_t)(int fd, int events, void *arg);
typedef void (*hpcf_process_data_callback_t)(int fd, void *arg);

#define MAX_BUF_SIZE (8 * 1024)

struct hpcf_event {
    int fd;
    int events;
    void *arg;
    int writable;
    int conn_closed;
    hpcf_event_callback_t callback;
    int active;
};

enum OTHER_TASK_STEP {
    OTHER_TASK_STEP_INIT = 0,
    OTHER_TASK_STEP_READ,
    OTHER_TASK_STEP_PROCESS,
    OTHER_TASK_STEP_WRITE,
    OTHER_TASK_STEP_FINISH,
    HPCF_OTHER_TASK_STEP_CLOSE,
};

// 其它任务队列
struct hpcf_other_task {
    struct list_head list;
    int fd;
    struct hpcf_event *hpcf_event;
    void *data;
    int data_len;
    enum OTHER_TASK_STEP step;           // 0: init, 1: read, 2: process, 3: write, 4: finish
    hpcf_process_data_callback_t read_data;
    hpcf_process_data_callback_t process_data;
    hpcf_process_data_callback_t write_data;
};


void hpcf_event_clear_1(struct hpcf_event *hev);

void hpcf_event_clear_n(struct hpcf_event *hev, int n);

struct hpcf_other_task *hpcf_new_other_task(
    int fd,
    struct hpcf_event *hev,
    hpcf_process_data_callback_t read_data,
    hpcf_process_data_callback_t process_data,
    hpcf_process_data_callback_t write_data);

void hpcf_free_other_task(struct hpcf_other_task *other_task);

#endif /* HPCF_EVENT */
