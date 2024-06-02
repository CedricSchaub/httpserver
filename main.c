#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
#include <malloc.h>
#include "http.h"
#include <signal.h>

#define RECV_BUFFER_SIZE 1000
#define SERVER_BACKLOG 10

static const int yes = 1;

BOOL shutdown_server = false;

void termination_handler(int signum) {
    printf("Signal %i received.\n", signum);
    shutdown_server = true;
}

int main(void) {

    // setup signal handler
    struct sigaction action = { .sa_handler = termination_handler, .sa_flags = 0 };
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL); 

    // setup tcp server
    struct sockaddr_storage incoming_addr;
    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("localhost", "4545", &hints, &result);

    int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(socket_fd, result->ai_addr, result->ai_addrlen);

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    printf("Started server listening on port: %s\n\n", "4545");
    listen(socket_fd, SERVER_BACKLOG);

    socklen_t addr_size = sizeof(incoming_addr);

    char recv_buffer[RECV_BUFFER_SIZE];
    char buffer[RECV_BUFFER_SIZE];

    int incoming_socket_fd;
    while( shutdown_server == false && (incoming_socket_fd = accept(socket_fd, (struct sockaddr *)&incoming_addr, &addr_size)) != -1 ) {
        ssize_t received_bytes;
        ssize_t total_received_bytes = 0;
        int request_status;
        http_request request = { .request_line = {.method = NULL, .request_target = NULL, .http_version = NULL, .complete = false } };
        printf("New incomming connection with fd: %i.\n\n", incoming_socket_fd);

        // set receive timeout for incoming socket
        struct timeval timeout;
        timeout.tv_sec = 15;
        timeout.tv_usec = 0;
        setsockopt(incoming_socket_fd, SOCK_STREAM, SO_RCVTIMEO, &timeout, sizeof(timeout));

        while( (received_bytes = recv(incoming_socket_fd, recv_buffer, 10, 0)) > 0) {
            memcpy(buffer + total_received_bytes, recv_buffer, received_bytes);
            total_received_bytes += received_bytes;

            request_status = parse_request(&request, buffer, total_received_bytes);

            if(request_status == COMPLETE_REQUEST || request_status == MALFORMED_REQUEST || request_status == INVALID_REQUEST) {
                break;
            }
        }

        // probably timeout
        if(received_bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            printf("Timeout time baby!\nClose connection with fd: %i\n\n", incoming_socket_fd);
            send(incoming_socket_fd, HTTP_TIMEOUT_RESPONSE, sizeof(HTTP_TIMEOUT_RESPONSE), 0);
        } else if(request_status == COMPLETE_REQUEST) {
            send(incoming_socket_fd, HTTP_DUMMY_RESPONSE, sizeof(HTTP_DUMMY_RESPONSE), 0);
        } else if(request_status == INVALID_REQUEST) {
            send(incoming_socket_fd, HTTP_ERROR_RESPONSE, sizeof(HTTP_ERROR_RESPONSE), 0);
        }
        // TODO: Add unknown error response

        free(request.request_line.request_target);
        free(request.request_line.method);
        free(request.request_line.http_version);

        printf("about to close incoming socket\n");
        // try to shutdown the socket so that the client does know what's gonna happen.
        // if I dont't do this, the client gets a Reset connection by peer error (curl e.x)
        shutdown(incoming_socket_fd, SHUT_RDWR);
        close(incoming_socket_fd);
    }

    printf("about to close server socket\n");
    freeaddrinfo(result);
    close(socket_fd);

    return 0;
}
