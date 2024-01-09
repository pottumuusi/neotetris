#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>

void
print_info(const char* msg)
{
    const char* program_name = "Pong";
    const char* print_level = "I";

    (void) printf("[%s|%s] %s\n", program_name, print_level, msg);
}

int
main(void)
{
    const int32_t socket_protocol = 0;
    const int32_t msg_size = 32;
    const char* sock_path;

    ssize_t bytecount;
    int32_t latest_error;
    int32_t sfd;
    int32_t ret;

    struct sockaddr_un address_server;
    struct sockaddr_un address_client;

    ret = 0;
    sfd = 0;
    bytecount = 0;
    latest_error = 0;
    sock_path = "/tmp/socket_local_pong";

    memset(&address_server, 0, sizeof(struct sockaddr_un));
    memset(&address_client, 0, sizeof(struct sockaddr_un));

    print_info("Starting up...");

    sfd = socket(AF_UNIX, SOCK_DGRAM, socket_protocol);
    if (-1 == sfd) {
        latest_error = errno;
        perror("Failed to create socket");
        goto error_exit;
    }

#ifndef NDEBUG
    printf("");
#endif // NDEBUG

    print_info("Shutting down...");
    ret = close(sfd);
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to close socket");
        goto error_exit;
    }

    print_info("Exiting...");
    return 0;

error_exit:
    return latest_error;
}
