#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "hpcf_connection.h"
#include "hpcf_event.h"

extern struct list_head g_all_connection_queue;

struct hpcf_connection *hpcf_new_connection(int fd,
                                             hpcf_event_callback_t read_callback,
                                             hpcf_event_callback_t write_callback,
                                             int accept)
{
    struct hpcf_connection *conn = 
        (struct hpcf_connection *)malloc(sizeof(struct hpcf_connection));
    
    if (conn == NULL) {
        return NULL;
    }
    if (!accept) {
        // 添加到所有连接链表中去
        list_add_tail(&conn->list, &g_all_connection_queue);
    }

    conn->fd = fd;
    conn->read_buffer = (char *)malloc(MAX_BUF_SIZE);
    if (conn->read_buffer == NULL) {
        goto err1;
    }
    conn->write_buffer = (char *)malloc(MAX_BUF_SIZE);
    if (conn->write_buffer == NULL) {
        goto err2;
    }
    conn->read_event = hpcf_new_event(fd, EPOLLIN, read_callback, accept, conn);
    if (conn->read_event == NULL) {
        goto err3;
    }
    conn->write_event = hpcf_new_event(fd, EPOLLOUT, write_callback, accept, conn);
    if (conn->write_event == NULL) {
        goto err4;
    }

    return conn;
err4:
    hpcf_free_event(conn->read_event);
err3:
    free(conn->write_buffer);
err2:
    free(conn->read_buffer);
err1:
    free(conn);
    return NULL;
}

// 关闭连接，但不释放此结构的内存，并从所有连接链表中删除
void hpcf_close_connection(struct hpcf_connection *conn)
{
    if (conn->list.prev != NULL) {
        list_del(&conn->list);
        memset(&conn->list, 0, sizeof(struct list_head));
    }
    close(conn->fd);
    conn->fd = -1;

    // 如果连接的事件在其它任务队列中，则删除
    hpcf_del_event_from_task_queue(conn->read_event);
    hpcf_del_event_from_task_queue(conn->write_event);
}

void hpcf_free_connection(struct hpcf_connection *conn)
{
    hpcf_close_connection(conn);
    free(conn->read_buffer);
    free(conn->write_buffer);
    free(conn->read_event);
    free(conn->write_event);
    if (conn->data != NULL) {
        free(conn->data);
        conn->data = NULL;
    }
    free(conn);
}