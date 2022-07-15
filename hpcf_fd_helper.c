#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>


void hpcf_set_fd_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

void hpcf_set_fd_block(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

int hpcf_try_lock_fd(int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return -1;
    } else {
        return 0;
    }
}

int hpcf_unlock_fd(int fd)
{
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return -1;
    } else {
        return 0;
    }
}