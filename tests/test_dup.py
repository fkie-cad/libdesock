"""
This file tests the dup*() implementations.

Target files:
    - libdesock/src/dup.c
"""

import ctypes

import desock
import helper

def dup23(method, *flags):
    s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
    n = method(s, s + 1, *flags)
    assert(n == s + 1)
    entry1 = desock.fd_entry()
    entry2 = desock.fd_entry()
    desock._debug_get_fd_table_entry(s, entry1)
    desock._debug_get_fd_table_entry(n, entry2)
    assert(entry1.domain == entry2.domain)
    assert(entry1.desock == entry2.desock)
    assert(entry1.listening == entry2.listening)
    assert(entry1.epfd == entry2.epfd)
    assert(entry1.ep_event.events == entry2.ep_event.events)
    assert(entry1.ep_event.data.u64 == entry2.ep_event.data.u64)
    desock.close(n)
    desock.close(s)
    
def dup23_invalid_new(method, *flags):
    s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
    n = desock.FD_TABLE_SIZE
    assert(method(s, n, *flags) == n)
    desock.close(n)
    desock.close(s)
    
def test_dup2_and_dup3():
    for method in [dup23_invalid_new, dup23]:
        method(desock.dup2)
        method(desock.dup3, 0)
