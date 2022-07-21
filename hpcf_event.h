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

// 此结构是放到任务队列中的
struct hpcf_event {
    struct list_head list;  // 用来添加到任务队列中
    int fd;
    int events;
    void *arg;              // 用来传递给回调函数，这里指向自己 struct hpcf_event;
    hpcf_event_callback_t callback;     // 如果在执行过程中发现连接已经关闭，则通过arg参数找到自己和连接中的其它任务并删除
    int accept;            // 是否是accept事件
    int closed;     // 连接是否关闭
    int posted;     // 任务是否已经提交到其它任务队列中
    int active;     // 回调函数是否激活，比如，当读完一次数据后，需要在处理数据和发送结果之后，才再次去读数据
    int ready;      // 事件是否可用，当ready时，表示缓冲区可读或可写
    int completed;  // 事件是否完成，当completed时，表示回调函数已正常执行
    void *data;     // 一般指向所属的连接 struct hpcf_connection;
};


struct hpcf_event *hpcf_new_event(int fd, int events,
    hpcf_event_callback_t callback, int accept, void *data);

void hpcf_free_event(struct hpcf_event *event);

void hpcf_add_event_to_task_queue(struct hpcf_event *hev);

void hpcf_del_event_from_task_queue(struct hpcf_event *hev);

// 执行队列中的任务
void hpcf_process_task_queue(struct list_head *task_queue);

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

// 下面这两个函数目前看来用不上，先不删除
void hpcf_event_clear_1(struct hpcf_event *hev);

void hpcf_event_clear_n(struct hpcf_event *hev, int n);

// struct hpcf_other_task *hpcf_new_other_task(
//     int fd,
//     struct hpcf_event *hev,
//     hpcf_process_data_callback_t read_data,
//     hpcf_process_data_callback_t process_data,
//     hpcf_process_data_callback_t write_data);

// void hpcf_free_other_task(struct hpcf_other_task *other_task);

#endif /* HPCF_EVENT */
