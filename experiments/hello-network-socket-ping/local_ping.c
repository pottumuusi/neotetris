#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>

#include "shared.h"
#include "util.h"

#define SIZE_MESSAGE_PING 4

const char* g_program_name = "Ping";

static int
prepare_for_bind_client(struct sockaddr_un* addr_ptr)
{
    int32_t sock_path_max_len;
    int32_t ret;

    char error_message[SIZE_ERROR_MESSAGE];
    char* sock_path;

    ret = 0;
    sock_path = 0;

    memset(error_message, 0, SIZE_ERROR_MESSAGE);

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
#endif // NDEBUG

    free(sock_path);
    sock_path = 0;

    return 0;
}

static void
construct_server_address(struct sockaddr_un* addr_ptr)
{
    memset(addr_ptr, 0, sizeof(struct sockaddr_un));
    addr_ptr->sun_family = AF_UNIX;
    strncpy(
            addr_ptr->sun_path,
            SOCK_PATH_SERVER,
            sizeof(addr_ptr->sun_path) - 1);
}

static int
send_and_echo_message(
        int32_t const * const sock_fd_ptr,
        struct sockaddr_un* address_server_ptr)
{
    char message[SIZE_MESSAGE_PING];
    int32_t ret;
    int32_t flags;

    ret = 0;
    flags = 0;
    memset(message, 0, SIZE_MESSAGE_PING);

    strncpy(message, "Ping", SIZE_MESSAGE_PING);

    ret = sendto(
            *(sock_fd_ptr),
            message,
            SIZE_MESSAGE_PING,
            flags,
            (struct sockaddr*) address_server_ptr,
            sizeof(struct sockaddr_un));
    if (-1 == ret) {
        perror("Failed to send message to server");
        return -1;
    }

    // TODO implement echoing

    return 0;
}

int main(void)
{
    const int32_t socket_protocol = 0;

    int32_t latest_error;
    int32_t sock_fd;
    int32_t ret;

    struct sockaddr_un address_client;
    struct sockaddr_un address_server;

    ret = 0;
    sock_fd = 0;

    memset(&address_client, 0, sizeof(struct sockaddr_un));
    memset(&address_server, 0, sizeof(struct sockaddr_un));

    print_info("Starting up");

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, socket_protocol);
    if (-1 == sock_fd) {
        latest_error = errno;
        perror("Failed to create socket");
        goto error_exit;
    }

    ret = prepare_for_bind_client(&address_client);
    if (-1 == ret) {
        print_error("Failed to prepare for bind");
        goto error_exit;
    }

    ret = bind(
            sock_fd,
            (struct sockaddr*) &address_client,
            sizeof(struct sockaddr_un));
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to bind socket");
        goto error_exit;
    }

#ifndef NDEBUG
    (void) printf(
            "Client socket bound to: %s\n",
            address_client.sun_path);
#endif // NDEBUG

    construct_server_address(&address_server);

    print_info("Sending and echoing a message");
    ret = send_and_echo_message(
            &sock_fd,
            &address_server);
    if (-1 == ret) {
        print_error("Message send and echo failed");
        (void) close(sock_fd);
        (void) remove(address_client.sun_path);
        goto error_exit;
    }

    print_info("Commencing client teardown");
    ret = close(sock_fd);
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to close socket");
        (void) remove(address_client.sun_path);
        goto error_exit;
    }

    ret = remove(address_client.sun_path);
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to remove socket file");
        goto error_exit;
    }

    print_info("Exiting");
    return 0;

error_exit:
    return latest_error;
}
