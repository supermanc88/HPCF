#ifndef HPCF_MAIN
#define HPCF_MAIN

#define HPCF_MAIN_VERSION "0.0.1"
#define HPCF_MAX_EVENTS 2048
#define HPCF_LISTEN_BACKLOG 511
#define HPCF_WORKER_PROCESS_NUM 4
#define HPCF_LISTEN_PORT 9899

void hpcf_read_event_callback(int client_fd, int events, void *arg);

void hpcf_write_event_callback(int client_fd, int events, void *arg);

void hpcf_read_data(int fd, void *arg);

void hpcf_process_data(int fd, void *arg);

void hpcf_write_data(int fd, void *arg);

#endif /* HPCF_MAIN */
