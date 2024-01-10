#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>

#include "util.h"

#define SIZE_ERROR_MESSAGE 256
#define SIZE_RECEIVE_BUFFER 512

const char* g_program_name = "Pong";

// TODO
// * write separate files for Unix domain sockets and internet domain sockets?

static int
prepare_for_bind(struct sockaddr_un* addr_ptr, const char* sock_path)
{
    char error_message[SIZE_ERROR_MESSAGE];
    int32_t ret;

    ret = 0;
    memset(error_message, 0, SIZE_ERROR_MESSAGE);

    if (strlen(sock_path) > sizeof(addr_ptr->sun_path) - 1) {
        sscanf(
                error_message,
                "Socket path is too long (%d characters)",
                strlen(sock_path));
        error_message[255] = '\0';
        print_error(error_message);
        return -1;
    }

    // Ensure there is no pre-existing file with wanted name
    ret = remove(sock_path);
    if (-1 == ret && ENOENT != errno) {
        perror("Failed to remove file");
        sscanf(
                error_message,
                "Failed to remove file, using path: %s",
                sock_path);
        error_message[255] = '\0';
        print_error(error_message);
        return -1;
    }

    memset(addr_ptr, 0, sizeof(struct sockaddr_un));

    addr_ptr->sun_family = AF_UNIX;

    // Using sizeof the whole struct had two bytes (short) extra when compared
    // to using sizeof sockaddr_un.sun_path. Checked from system header files.
    strncpy(addr_ptr->sun_path, sock_path, sizeof(struct sockaddr_un) - 1);

    return 0;
}

static int
receive_message(
        int32_t const * const sock_fd_ptr,
        struct sockaddr_un* address_client_ptr)
{
    socklen_t len;
    ssize_t bytes_received;

    char receive_buffer[SIZE_RECEIVE_BUFFER];

    len = 0;
    bytes_received = 0;
    memset(receive_buffer, 0, SIZE_RECEIVE_BUFFER);

    len = sizeof(struct sockaddr_un);
    bytes_received = recvfrom(
            *(sock_fd_ptr),
            receive_buffer,
            SIZE_RECEIVE_BUFFER,
            0,
            (struct sockaddr*) &address_client_ptr,
            &len);
    if (-1 == bytes_received) {
        perror("Failed to recvfrom");
        return -1;
    }

#ifndef NDEBUG
    (void) printf(
            "Server received %ld bytes from: %s\n",
            (long) bytes_received,
            address_client_ptr->sun_path);
#endif // NDEBUG

    return 0;
}

int
main(void)
{
    const int32_t socket_protocol = 0;
    const int32_t msg_size = 32;
    const char* sock_path;

    ssize_t bytecount;
    int32_t latest_error;
    int32_t sock_fd;
    int32_t ret;

    struct sockaddr_un address_server;
    struct sockaddr_un address_client;

    ret = 0;
    sock_fd = 0;
    bytecount = 0;
    latest_error = 0;
    sock_path = "/tmp/socket_local_pong";

    print_info("Starting up");

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, socket_protocol);
    if (-1 == sock_fd) {
        latest_error = errno;
        perror("Failed to create socket");
        goto error_exit;
    }

    ret = prepare_for_bind(&address_server, sock_path);
    if (-1 == ret) {
        print_error("Failed to prepare for bind");
        goto error_exit;
    }

    ret = bind(
            sock_fd,
            (struct sockaddr*) &address_server,
            sizeof(struct sockaddr_un));
    if (-1 == ret) {
        latest_error = errno;
        perror("Failed to bind socket");
        goto error_exit;
    }

#ifndef NDEBUG
    (void) printf(
            "Server bound to: %s\n",
            address_server.sun_path);
#endif // NDEBUG

    print_info("Starting to receive messages");
    for (;;) {
        receive_message(&sock_fd, &address_client);
    }

#if 0
    // TODO Do not make this server bind a client socket
    ret = prepare_for_?(&address_client, sock_path);
    if () {
    }
#endif

    print_info("Commencing server teardown");
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
