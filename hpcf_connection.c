#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "hpcf_core.h"

int hpcf_open_listening_socket()
{
    // create listen socketaddr_in
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(HPCF_LISTEN_PORT);

    // 创建listen socket
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        printf("socket error\n");
        return HPCF_ERR;
    }

    // reuse port
    int reuseport = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport)) < 0) {
        printf("setsockopt error\n");
        close(listen_socket);
        return HPCF_ERR;
    }

    // bind port
    if (bind(listen_socket, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1) {
        printf("bind error\n");
        close(listen_socket);
        return HPCF_ERR;
    }

    // listen
    if (listen(listen_socket, HPCF_LISTEN_BACKLOG) == -1) {
        printf("listen error\n");
        close(listen_socket);
        return HPCF_ERR;
    }

    return HPCF_OK;
}