#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"
#include "desock.h"
#include "stub_sockaddr.h"

static const struct sockaddr_in STUB_IPv4 = {
    .sin_family = AF_INET,
    .sin_port = 53764,
    .sin_addr.s_addr = 0x100007f
};

static const struct sockaddr_in6 STUB_IPv6 = {
    .sin6_family = AF_INET6,
    .sin6_port = 53764,
    .sin6_flowinfo = 0,
    .sin6_addr.s6_addr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    .sin6_scope_id = 0
};

/*
 * Given an fd that is being desocketed, fill the given sockaddr structure
 * with a stub from above.
 */
void fill_sockaddr (int fd, struct sockaddr* addr, socklen_t* addr_len) {
    if (UNLIKELY(!addr || !addr_len)) {
        return;
    }
    
    switch (fd_table[fd].domain) {
        case AF_INET: {
            if (UNLIKELY(*addr_len != sizeof(STUB_IPv4))) {
                _error("fill_sockaddr() got an invalid size of sockaddr_in structure: %lu", (size_t) *addr_len);
            }
            __builtin_memcpy(addr, (void*) &STUB_IPv4, sizeof(STUB_IPv4));
            break;
        }

        case AF_INET6: {
            if (UNLIKELY(*addr_len != sizeof(STUB_IPv6))) {
                _error("fill_sockaddr() got an invalid size of sockaddr_in6 structure: %lu", (size_t) *addr_len);
            }
            __builtin_memcpy(addr, (void*) &STUB_IPv6, sizeof(STUB_IPv6));
            break;
        }

        default: {
            __builtin_unreachable();
        }
    }
}
