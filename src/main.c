#include <stdio.h>
#include <stdlib.h>

#include "util.h"

#ifdef SHARED
VISIBLE const char __libdesock_interpreter[] __attribute__ ((section (".interp"))) = INTERPRETER;

__attribute__((noreturn))
int __libdesock_main (void) {
    printf(
        "libdesock.so: A fast desocketing library built for fuzzing\n"
        "\n"
        "This library can desock\n"
        "    servers = "
#ifdef DESOCK_BIND
            "yes"
#else
            "no"
#endif
        "\n"
        "    clients = "
#ifdef DESOCK_CONNECT
            "yes"
#else
            "no"
#endif
        "\n\n"
        "Compilation options:\n"
        "    - DEBUG = "
#ifdef DEBUG
            "yes"
#else
            "no"
#endif
        "\n"
        "    - MAX_CONNS = %d\n"
        "    - FD_TABLE_SIZE = %d\n"
        "    - ARCH = %s\n"
        "\n"
        "Support for multiple requests: "
#ifdef MULTI_REQUEST
        "yes"
#else
        "no"
#endif
        "\n"
#ifdef MULTI_REQUEST
        "    delimiter = \"%s\"\n"
#endif
        "\n"
        "Use this with LD_PRELOAD=libdesock.so on a network application\n"
        "or with AFL_PRELOAD=libdesock.so when fuzzing with AFL.\n", 
        MAX_CONNS, FD_TABLE_SIZE, DESOCKARCH
#ifdef MULTI_REQUEST
        , REQUEST_DELIMITER
#endif
    );

    while (1) {
        _Exit(0);
    }
}

#endif
