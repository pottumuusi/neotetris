#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>

#include "util.h"

const char* g_program_name = "Ping";

static int
prepare_for_bind(struct sockaddr_un* addr_ptr)
{
    int32_t sock_path_max_len;
    int32_t ret;

    char error_message[SIZE_ERROR_MESSAGE];
    char* sock_path;

    ret = 0;
    sock_path = 0;

    sock_path_max_len = sizeof(addr_ptr->sun_path) - 1;
    sock_path = (char*) calloc(sock_path_max_len, sizeof(char));
    if (0 == sock_path) {
        snprintf(
                error_message,
                SIZE_ERROR_MESSAGE,
                "Failed to allocate memory for socket path");
        print_error(error_message);
        return -1;
    }

    memset(addr_ptr, 0, sizeof(struct sockaddr_un));

    addr_ptr->sun_family = AF_UNIX;

    // Construct client socket path with the help of PID
    snprintf(
            sock_path,
            sock_path_max_len,
            "/tmp/%s.%ld",
            g_program_name,
            (long) getpid());

    strncpy(addr_ptr->sun_path, sock_path, sock_path_max_len);

#ifndef NDEBUG
    (void) printf("Constructed socket path: %s\n", addr_ptr->sun_path);
#endif

    free(sock_path);
    sock_path = 0;

    return 0;
}

int main(void)
{
    const int32_t socket_protocol = 0;

    int32_t latest_error;
    int32_t sock_fd;
    int32_t ret;

    struct sockaddr_un address_client;

    ret = 0;
    sock_fd = 0;

    print_info("Starting up");

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, socket_protocol);
    if (-1 == sock_fd) {
        latest_error = errno;
        perror("Failed to create socket");
        goto error_exit;
    }

    ret = prepare_for_bind(&address_client);
    if (-1 == ret) {
        print_error("Failed to prepare for bind");
        goto error_exit;
    }

    print_info("Commencing client teardown");
    ret = close(sock_fd);
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to close socket");
        goto error_exit;
    }

    // TODO remove() client socket file

    print_info("Exiting");
    return 0;

error_exit:
    return latest_error;
}
