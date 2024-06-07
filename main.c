#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "arena.h"
#include "http.h"
#include "args.h"
#include <signal.h>

#define RECV_BUFFER_SIZE 1000
#define SERVER_BACKLOG 10
#define MB(A) A*1024

static const int yes = 1;

BOOL shutdown_server = false;

void termination_handler(int signum) {
    printf("Signal %i received.\n", signum);
    shutdown_server = true;
}

void send_file(const char *path) {
    char new_path[255] = {"/tmp"};
    strcat(&new_path, path);

    FILE *fp = fopen(new_path, "r");
    // TODO: implement quick and dirty

    fclose(fp);
}

int main(int argc, char *argv[]) {
    // setup arena
    memory_arena arena = { .base = NULL, .offset = 0, .size = 0 };
    initialize_arena(&arena, MB(50));

    // parse command line arguments
    http_server_args server_args = create_default_args();
    parse_args(&server_args, argv, argc);

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

    getaddrinfo(server_args.host, server_args.port, &hints, &result);

    int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(socket_fd, result->ai_addr, result->ai_addrlen);

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    listen(socket_fd, SERVER_BACKLOG);

    printf("Started server %s:%s\n", server_args.host, server_args.port);

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

            request_status = parse_request(&arena, &request, buffer, total_received_bytes);

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

        printf("about to close incoming socket\n");
        reset_arena(&arena);
        // try to shutdown the socket so that the client does know what's gonna happen.
        // if I dont't do this, the client gets a Reset connection by peer error (curl e.x)
        shutdown(incoming_socket_fd, SHUT_RDWR);
        close(incoming_socket_fd);
    }

    printf("about to close server socket\n");
    freeaddrinfo(result);
    close(socket_fd);
    free_arena(&arena);

    return 0;
}
