#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "hpcf_core.h"

void hpcf_worker_process_cycle(void *data)
{
    // worker进程流程
    for ( ;; ) {
        // hpcf_process_events_and_timers();
    }

}


pid_t hpcf_spawn_process()
{
    pid_t pid;
    pid = fork();

    switch (pid) {
        case -1:
            printf("fork error\n");
            return HPCF_ERR;
        case 0:
            // child process
            hpcf_worker_process_cycle(NULL);
            break;
        default:
            break;
    }
    return pid;
}

void hpcf_start_worker_process(int worker_count)
{
    int i;
    for (i = 0; i < worker_count; i++) {
        hpcf_spawn_process();
    }
}

void hpcf_master_process_cycle()
{
    hpcf_start_worker_process(4);
}