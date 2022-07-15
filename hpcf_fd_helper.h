#ifndef HPCF_FD_HELPER
#define HPCF_FD_HELPER

void hpcf_set_fd_nonblock(int fd);

void hpcf_set_fd_block(int fd);

int hpcf_try_lock_fd(int fd);

int hpcf_unlock_fd(int fd);

#endif /* HPCF_FD_HELPER */
