#include "args.h"
#include <getopt.h>
#include "stdio.h"
#include <string.h>
#include <stdlib.h>

static struct option const long_options[] = {
    { "host",    required_argument, NULL, 'h' },
    { "port",    required_argument, NULL, 'p' },
    { "verbose", no_argument,       NULL, 'v' },
    { NULL,                0,       NULL,   0 },
};

http_server_args create_default_args() {
    http_server_args _default = { .host = "localhost", .port = "4545", .verbose = 0 };
    return _default;
}

void parse_args(http_server_args *args, char *argv[], int argc) {
    // disable error message if unknown option is found
    opterr = 0;

    int option_found;
    while( (option_found = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch(option_found) {
            case 'h':
                args->host = optarg;
                break;
            case 'p':
                args->port = optarg;
                break;
            case 'v':
                args->verbose = 1;
                break;
            default:
                printf("Only long options --port <value> and|or --verbose allowed.\n");
                exit(1);
                break;
        }
    }
}
