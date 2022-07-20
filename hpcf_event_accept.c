#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "hpcf_core.h"

int hpcf_trylock_fd(int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return HPCF_ERR;
    }
    return HPCF_OK;
}

int hpcf_unlock_fd(int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(struct flock));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return HPCF_ERR;
    }
    return HPCF_OK;
}

int hpcf_trylock_accept_mutex()
{
    if (hpcf_trylock_fd(HPCF_ACCEPT_MUTEX_FD) == HPCF_ERR) {
        return HPCF_ERR;
    }
    return HPCF_OK;
}