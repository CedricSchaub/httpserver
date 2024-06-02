#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


const char HTTP_DUMMY_RESPONSE[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nMy payload is the shit\r\n";
const char HTTP_ERROR_RESPONSE[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nYou issued a malformed or illegal request.\r\n";
const char HTTP_TIMEOUT_RESPONSE[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nTimed out bitch.\r\n";


#define true 1
#define false 0

typedef unsigned int BOOL;

typedef struct {
    char *method;
    char *request_target;
    char *http_version;
    BOOL complete;
} http_request_line;

typedef struct {
    http_request_line request_line;
    // headers
} http_request;


static void print_http_request(const http_request_line *request) {
    printf("http_request->method: %s\nhttp_request->request_target: %s\nhttp_request->http_version: %s\n\n", request->method, request->request_target, request->http_version);
}

unsigned validate_request_line(http_request_line *request) {
    if(strcmp(request->method, "GET") != 0 && strcmp(request->method, "HEAD") != 0) {
        printf("request->method: %s != GET|HEAD\n", request->method);
        return false;
    }

    if(strcmp(request->http_version, "HTTP/1.1") != 0) {
        printf("request->http_version: %s != HTTP/1.1\n", request->http_version);
        return false;
    }

    return true;
}

// check if \r\n can be found in the buffer and return position of \n
int find_crlf(const char recv_buffer[], int recv_buffer_size) {
    assert(recv_buffer_size >= 2);

    int index = 0;

    char prev_char = recv_buffer[0];
    for(;index < recv_buffer_size; ++index) {
        if(prev_char == '\r' && recv_buffer[index] == '\n') {
            return index;
        }
        prev_char = recv_buffer[index];
    }

    return -1;
}

int strnchr(const char buffer[], int buffer_size, int offset, char c) {
    int index = offset;
    BOOL found = false;
    for(; index < buffer_size; ++index) {
        if(buffer[index] == c) {
            found = true;
            break;
        }
    }

    return found ? index : -1;
}

#define INCOMPLETE_REQUEST_LINE 0
#define COMPLETE_REQUEST_LINE 1
#define COMPLETE_REQUEST 2
#define MALFORMED_REQUEST -1
#define INVALID_REQUEST -2 // For example http method other than GET|HEAD or wrong http version

static int parse_request_line(http_request_line *request, const char recv_buffer[], int recv_buffer_size) {
    int crlf_index = find_crlf(recv_buffer, recv_buffer_size);
    int request_line_end = crlf_index - 2;
    if(request_line_end < 0) return INCOMPLETE_REQUEST_LINE;

    int first_whitespace_index = strnchr(recv_buffer, request_line_end, 0, ' ');
    // GET_\r\n <--- avoid such cases
    if(first_whitespace_index + 1 >= request_line_end || first_whitespace_index == -1) return MALFORMED_REQUEST;

    int second_whitespace_index = strnchr(recv_buffer, request_line_end, first_whitespace_index+1, ' ');
    // GET__\r\n <--- avoid such cases
    if(second_whitespace_index + 1 >= request_line_end || second_whitespace_index == -1) return MALFORMED_REQUEST;

    request->method = (char *) malloc(sizeof(char) * (first_whitespace_index));
    memcpy(request->method, recv_buffer, first_whitespace_index);
    request->method[first_whitespace_index] = '\0';

    request->request_target = (char *) malloc(sizeof(char) * second_whitespace_index - first_whitespace_index);
    memcpy(request->request_target, recv_buffer+first_whitespace_index+1, second_whitespace_index - first_whitespace_index);
    request->request_target[second_whitespace_index - first_whitespace_index] = '\0';

    request->http_version = (char *) malloc(sizeof(char) * request_line_end - second_whitespace_index);
    memcpy(request->http_version, recv_buffer+second_whitespace_index+1, request_line_end-second_whitespace_index);
    request->http_version[request_line_end - second_whitespace_index] = '\0';

    request->complete = true;

    return COMPLETE_REQUEST_LINE;
}

int parse_request(http_request *request, const char recv_buffer[], int recv_buffer_size) {
    int status;

    if(!request->request_line.complete) {
        status = parse_request_line(&request->request_line, recv_buffer, recv_buffer_size);
        if(status == MALFORMED_REQUEST || status == INCOMPLETE_REQUEST_LINE) return status;
    }

    BOOL valid_request = validate_request_line(&request->request_line);

    if(!valid_request) return INVALID_REQUEST;

    // for debug
    print_http_request(&request->request_line);

    // parse header
    return COMPLETE_REQUEST;
}

#endif
