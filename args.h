#ifndef ARGS_H
#define ARGS_H

typedef struct {
  char *host;
  char *port;
  int verbose;
} http_server_args;

http_server_args create_default_args();
void parse_args(http_server_args *args, char *argv[], int argc);

#endif
