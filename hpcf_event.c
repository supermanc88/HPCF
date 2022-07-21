#include "hpcf_event.h"
#include <string.h>

extern struct list_head g_other_task_queue;

void hpcf_event_clear_1(struct hpcf_event *hev)
{
    hev->fd = -1;
    hev->events = 0;
    hev->arg = NULL;
    hev->callback = NULL;
    hev->active = 0;
}


void hpcf_event_clear_n(struct hpcf_event *hev, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        hpcf_event_clear_1(&hev[i]);
    }
}

// struct hpcf_other_task *hpcf_new_other_task(
//     int fd,
//     struct hpcf_event *hev,
//     hpcf_process_data_callback_t read_data,
//     hpcf_process_data_callback_t process_data,
//     hpcf_process_data_callback_t write_data)
// {
//     struct hpcf_other_task *other_task = (struct hpcf_other_task *)malloc(sizeof(struct hpcf_other_task));
//     if (other_task == NULL) {
//         perror("malloc error");
//         exit(1);
//     }
//     other_task->fd = fd;
//     other_task->hpcf_event = hev;
//     other_task->step = OTHER_TASK_STEP_INIT;
//     other_task->data = (void *) malloc(MAX_BUF_SIZE);
//     other_task->data_len = 0;
//     other_task->read_data = read_data;
//     other_task->process_data = process_data;
//     other_task->write_data = write_data;
//     return other_task;
// }

// void hpcf_free_other_task(struct hpcf_other_task *other_task)
// {
//     if (other_task->data != NULL) {
//         free(other_task->data);
//     }
//     free(other_task);
// }


struct hpcf_event *hpcf_new_event(int fd, int events,
    hpcf_event_callback_t callback, int accept, void *data)
{
    struct hpcf_event *hev = 
        (struct hpcf_event *)malloc(sizeof(struct hpcf_event));
    if (hev == NULL) {
        perror("malloc error");
        return NULL;
    }
    memset(hev, 0, sizeof(struct hpcf_event));

    // init event list
    memset(&hev->list, 0, sizeof(struct list_head));

    hev->fd = fd;
    hev->events = events;
    hev->arg = (void *)hev;
    hev->callback = callback;
    hev->accept = accept;
    hev->data = data;
    return hev;
}

void hpcf_add_event_to_task_queue(struct hpcf_event *hev)
{
    // 如果event中的list没有在链表中，则添加到链表中
    if (hev->list.prev == NULL) {
        list_add_tail(&hev->list, &g_other_task_queue);
    }
}

void hpcf_del_event_from_task_queue(struct hpcf_event *hev)
{
    // 如果event中的list在链表中，则删除
    if (hev->list.prev != NULL) {
        list_del(&hev->list);
        memset(&hev->list, 0, sizeof(struct list_head));
    }
}


void hpcf_free_event(struct hpcf_event *event)
{
    // 先从其它任务中断链
    hpcf_del_event_from_task_queue(event);
    // 再释放event
    free(event);
}

void hpcf_process_task_queue(struct list_head *task_queue)
{
    // 遍历链表，处理链表中的任务
    struct list_head *pos, *n;
    if (list_empty(task_queue)) {
        return;
    }

    list_for_each_safe(pos, n, task_queue) {
        struct hpcf_event *hev = list_entry(pos, struct hpcf_event, list);
        list_del(pos);
        hev->callback(hev->fd, hev->events, hev->arg);
    }
}