"""
This file tests the select() functionality.

Target files:
    - libdesock/src/select.c
"""

import ctypes

import helper
import desock

def test_invalid():
    s = desock.socket(desock.AF_INET, desock.SOCK_DGRAM, 0)
    s1 = desock.dup2(s, desock.FD_TABLE_SIZE)
    rfds = desock.fd_set()
    desock.FD_ZERO(rfds)
    desock.FD_SET(s1, rfds)
    wfds = desock.fd_set()
    desock.FD_ZERO(wfds)
    desock.FD_SET(s1, wfds)
    assert(desock.select(s1 + 1, rfds, wfds, None, None) == 0)
    desock.close(s1)
    desock.close(s)
    assert(not desock.FD_ISSET(s1, rfds))
    assert(not desock.FD_ISSET(s1, wfds))

def _check_return_val(n, r, w, e):
    s = 0
    for i in range(desock.FD_SETSIZE):
        s += int(desock.FD_ISSET(i, r))
        s += int(desock.FD_ISSET(i, w))
        s += int(desock.FD_ISSET(i, e))
    assert(s == n)
    
def _read(fd):
    buf = ctypes.create_string_buffer(1024)
    n = desock.recv(fd, buf, ctypes.sizeof(buf), 0)
    return buf[:n]

def _simulate_server(s, method, *args):
    input = b""
    output = ctypes.create_string_buffer(b"fuck off")
    run = True
    client = None
    nfds = s + 1
    read_fds = desock.fd_set()
    write_fds = desock.fd_set()
    error_fds = desock.fd_set()
    
    desock.FD_ZERO(read_fds)
    desock.FD_ZERO(write_fds)
    desock.FD_ZERO(error_fds)
    
    desock.FD_SET(s, read_fds)
    desock.FD_SET(s, error_fds)
    
    while run:
        tmp_r_fds = desock.fd_set()
        tmp_w_fds = desock.fd_set()
        tmp_e_fds = desock.fd_set()
        desock.FD_COPY(tmp_r_fds, read_fds)
        desock.FD_COPY(tmp_w_fds, write_fds)
        desock.FD_COPY(tmp_e_fds, error_fds)
        
        n = method(nfds, tmp_r_fds, tmp_w_fds, tmp_e_fds, *args)
        _check_return_val(n, tmp_r_fds, tmp_w_fds, tmp_e_fds)
        
        for i in range(desock.FD_SETSIZE):
            if desock.FD_ISSET(i, tmp_r_fds):
                if i == s:
                    client = desock.accept(s, None, None)
                    desock.FD_SET(client, read_fds)
                    desock.FD_SET(client, write_fds)
                    desock.FD_SET(client, error_fds)
                    nfds = max(nfds - 1, client) + 1
                elif i == client:
                    b = _read(client)
                    if b:
                        input += b
                    else:
                        desock.FD_CLR(client, read_fds)
                        run = False
                else:
                    assert(False)
                    
            if desock.FD_ISSET(i, tmp_w_fds):
                if i == s:
                    assert(False)
                elif i == client:
                    desock.send(client, output, ctypes.sizeof(output) - 1, 0)
                    desock.FD_CLR(client, write_fds)
                else:
                    assert(False)
                    
            if desock.FD_ISSET(i, tmp_e_fds):
                assert(False)
                
    desock.close(client)
    return input

def test_select():
    data = b"test123456"
    for method, args in [(desock.select, [None]), (desock.pselect, [None, None])]:
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
