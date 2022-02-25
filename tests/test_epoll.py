"""
This file tests the epoll() implementation.

Target files:
    - libdesock/src/epoll.c
"""

import os
import ctypes
import random

import helper
import desock

def _read(fd):
    buf = ctypes.create_string_buffer(1024)
    n = desock.recv(fd, buf, ctypes.sizeof(buf), 0)
    return buf[:n]

def do_syscall(method, *args):
    s = desock.socket(desock.AF_INET, desock.SOCK_DGRAM, 0)
    e = desock.epoll_create()
    c = desock.dup2(s, desock.FD_TABLE_SIZE)
    event = desock.epoll_event()
    event.events = desock.EPOLLOUT
    event.data.fd = c
    desock.epoll_ctl(e, desock.EPOLL_CTL_ADD, c, event)
    events = (desock.epoll_event * 1)()
    assert(method(e, events, 1, *args) == 1)
    assert(events[0].events & (~desock.EPOLLOUT) == 0)
    assert(events[0].data.fd == c)
    desock.close(c)
    desock.close(e)
    desock.close(s)
    
def test_syscalls():
    do_syscall(desock.epoll_wait, -1)
    do_syscall(desock.epoll_pwait, -1, None)
    try:
        do_syscall(desock.epoll_pwait2, None, None)
    except desock.DesockError as e:
        assert(str(e) == "Function not implemented")
    else:
        assert(False)

def test_oneshot():
    s = desock.socket(desock.AF_INET, desock.SOCK_DGRAM, 0)
    desock.bind(s, None, 0)
    desock.listen(s, 0)
    e = desock.epoll_create()
    event = desock.epoll_event()
    event.events = desock.EPOLLIN | desock.EPOLLONESHOT
    event.data.u64 = 0
    desock.epoll_ctl(e, desock.EPOLL_CTL_ADD, s, event)
    fd_entry = desock.fd_entry()
    events = (desock.epoll_event * 1)()
    assert(desock.epoll_wait(e, events, 1, -1) == 1)
    assert(events[0].events & (~desock.EPOLLIN) == 0)
    desock._debug_get_fd_table_entry(s, fd_entry)
    assert(fd_entry.epfd == -1)
    c = desock.accept(s, None, None)
    desock.epoll_ctl(e, desock.EPOLL_CTL_ADD, c, event)
    assert(desock.epoll_wait(e, events, 1, -1) == 1)
    fd_entry = desock.fd_entry()
    desock._debug_get_fd_table_entry(c, fd_entry)
    assert(fd_entry.epfd == -1)
    desock.close(c)
    desock.close(s)
    desock.close(e)

def do_epoll(method, *args):
    data = b"yeeha"
    input = b""
    client = None
    ready_list = (desock.epoll_event * 2)()
    num_put_out = 0
    maxevents = 1
    run = True
    
    with helper.StdinPipe() as pipe:
        pipe.write(data)
        pipe.close()
        s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
        desock.bind(s, None, 0)
        desock.listen(s, 0)
        epfd = desock.epoll_create()
        
        event = desock.epoll_event()
        event.events = desock.EPOLLIN | desock.EPOLLOUT | desock.EPOLLERR | desock.EPOLLHUP
        event.data.u32 = 0x1337
        desock.epoll_ctl(epfd, desock.EPOLL_CTL_ADD, s, event)
        
        event = desock.epoll_event()
        event.events = desock.EPOLLIN | desock.EPOLLOUT | desock.EPOLLERR | desock.EPOLLHUP
        event.data.fd = s
        desock.epoll_ctl(epfd, desock.EPOLL_CTL_MOD, s, event)
        
        while run:
            r = method(epfd, ready_list, maxevents, *args)
            assert(0 < r <= 2)
            
            for i in range(r):
                if ready_list[i].events & ~(desock.EPOLLIN | desock.EPOLLOUT):
                    assert(False)
                
                if ready_list[i].events & desock.EPOLLIN:
                    if ready_list[i].data.fd == s:
                        assert(client is None)
                        client = desock.accept(s, None, None)
                        event = desock.epoll_event()
                        event.events = desock.EPOLLIN | desock.EPOLLOUT | desock.EPOLLERR | desock.EPOLLHUP
                        event.data.fd = client
                        desock.epoll_ctl(epfd, desock.EPOLL_CTL_MOD, client, event)
                        maxevents += 1
                    elif ready_list[i].data.fd == client:
                        b = _read(client)
                        if b:
                            input += b
                        else:
                            run = False
                    else:
                        print(f"data = {ready_list[i].data.u64}")
                        assert(False)
                
                if ready_list[i].events & desock.EPOLLOUT:
                    if ready_list[i].data.fd == client:
                        num_put_out += 1
                        event = desock.epoll_event()
                        event.events = desock.EPOLLIN | desock.EPOLLERR | desock.EPOLLHUP
                        event.data.fd = client
                        desock.epoll_ctl(epfd, desock.EPOLL_CTL_MOD, client, event)
                    else:
                        assert(False)
        
        desock.epoll_ctl(epfd, desock.EPOLL_CTL_DEL, s, None)
        desock.close(s)
        desock.close(epfd)
        desock.close(client)
    assert(num_put_out == 1)
    assert(input == data)
    
def test_epoll():
    do_epoll(desock.epoll_wait, -1)
    do_epoll(desock.epoll_pwait, -1, None)
    do_epoll(desock.epoll_pwait2, None, None)
