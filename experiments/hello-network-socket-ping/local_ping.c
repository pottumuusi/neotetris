#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>

#include "util.h"

const char* g_program_name = "Ping";

int main(void)
{
    const int32_t socket_protocol = 0;

    int32_t latest_error;
    int32_t sock_fd;
    int32_t ret;

    ret = 0;
    sock_fd = 0;

    print_info("Starting up");

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, socket_protocol);
    if (-1 == sock_fd) {
        latest_error = errno;
        perror("Failed to create socket");
        goto error_exit;
    }

    print_info("Commencing client teardown");
    ret = close(sock_fd);
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to close socket");
        goto error_exit;
    }

    print_info("Exiting");
    return 0;

error_exit:
    return latest_error;
}
