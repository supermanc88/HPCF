#include "hpcf_event.h"

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

struct hpcf_other_task *hpcf_new_other_task(
    int fd,
    struct hpcf_event *hev,
    hpcf_process_data_callback_t read_data,
    hpcf_process_data_callback_t process_data,
    hpcf_process_data_callback_t write_data)
{
    struct hpcf_other_task *other_task = (struct hpcf_other_task *)malloc(sizeof(struct hpcf_other_task));
    if (other_task == NULL) {
        perror("malloc error");
        exit(1);
    }
    other_task->fd = fd;
    other_task->hpcf_event = hev;
    other_task->step = OTHER_TASK_STEP_INIT;
    other_task->data = (void *) malloc(MAX_BUF_SIZE);
    other_task->data_len = 0;
    other_task->read_data = read_data;
    other_task->process_data = process_data;
    other_task->write_data = write_data;
    return other_task;
}

void hpcf_free_other_task(struct hpcf_other_task *other_task)
{
    if (other_task->data != NULL) {
        free(other_task->data);
    }
    free(other_task);
}