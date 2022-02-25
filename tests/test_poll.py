"""
This file tests the poll() functionality.

Target files:
    - libdesock/src/poll.c
"""

import ctypes

import helper
import desock

def do_syscall(method, *args):
    s = desock.socket(desock.AF_INET, desock.SOCK_DGRAM, 0)
    s1 = desock.dup2(s, desock.FD_TABLE_SIZE)
    pollfds = (desock.pollfd * 1)()
    pollfds[0].fd = s1
    pollfds[0].events = desock.POLLOUT
    pollfds[0].revents = 0
    assert(method(pollfds, 1, *args) == 1)
    desock.close(s1)
    desock.close(s)
    
def test_syscall():
    do_syscall(desock.poll, -1)
    do_syscall(desock.ppoll, None, None)

def _read(fd):
    buf = ctypes.create_string_buffer(1024)
    n = desock.recv(fd, buf, ctypes.sizeof(buf), 0)
    return buf[:n]

def _check_ret_val(r, pollfds, nfds):
    assert(r > 0)
    s = 0
    for i in range(nfds):
        if pollfds[i].revents != 0:
            s += 1
    assert(s == r)

def _simulate_server(s, method, *args):
    input = b""
    client = None
    nfds = 1
    pollfds = (desock.pollfd * 2)()
    events = desock.POLLIN | desock.POLLERR | desock.POLLHUP | desock.POLLOUT
    run = True
    output = ctypes.create_string_buffer(b"fuck off")
    
    pollfds[0].fd = s
    pollfds[0].events = events
    pollfds[0].revents = 0
    
    while run:
        r = method(pollfds, nfds, *args)
        _check_ret_val(r, pollfds, nfds)
        
        for i in range(nfds):
            assert(pollfds[i].revents & (~events) == 0)
            
            if (pollfds[i].revents & desock.POLLERR) or (pollfds[i].revents & desock.POLLHUP):
                assert(False)
            
            if pollfds[i].revents & desock.POLLIN:
                if pollfds[i].fd == s:
                    client = desock.accept(s, None, None)
                    pollfds[1].fd = client
                    pollfds[1].events = events
                    pollfds[1].revents = 0
                    nfds += 1
                elif pollfds[i].fd == client:
                    b = _read(client)
                    if b:
                        input += b
                    else:
                        run = False
                else:
                    assert(False)
            
            if pollfds[i].revents & desock.POLLOUT:
                if pollfds[i].fd == client:
                    desock.send(client, output, ctypes.sizeof(output) - 1, 0)
                    pollfds[i].events &= (~desock.POLLOUT)
                else:
                    assert(False)
    
    desock.close(client)
    return input

def test_poll():
    data = b"test123456"
    for method, args in [(desock.poll, [-1]), (desock.ppoll, [None, None])]:
        for domain in helper.all_domains:
            for type in helper.all_types:
                with helper.StdinPipe() as in_pipe, helper.StdoutPipe() as out_pipe:
                    in_pipe.write(data)
                    in_pipe.close()
                    s = desock.socket(domain, type, 0)
                    desock.bind(s, None, 0)
                    desock.listen(s, 10)
                    assert(_simulate_server(s, method, *args) == data)
                    assert(out_pipe.read(8) == b"fuck off")
                    desock.close(s)
