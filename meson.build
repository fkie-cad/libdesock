project('libdesock', 'c')

if get_option('interpreter') == ''
    r = run_command('sh', '-c', 'readelf -l /bin/ls | grep "program interpreter" | cut -d: -f2  | sed -e "s/^.//" -e "s/.$//"')
    interpreter = r.stdout().strip()
else
    interpreter = get_option('interpreter')
endif

args = [
    '-O2',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wno-unused-parameter',
    '-Wno-sign-compare',
    '-D DESOCKARCH="@0@"'.format(get_option('arch')),
    '-D MAX_CONNS=1',
    '-D FD_TABLE_SIZE=@0@'.format(get_option('fd_table_size')),
    '-D INTERPRETER="@0@"'.format(interpreter)
]

if get_option('debug_desock')
    args += '-D DEBUG'
endif
    
if get_option('desock_client')
    args += '-D DESOCK_CONNECT'
endif
    
if get_option('desock_server')
    args += '-D DESOCK_BIND'
endif

libdesock_shared = shared_library('desock', 
    sources : [
        'libdesock/src/accept.c',
        'libdesock/src/bind.c',
        'libdesock/src/close.c',
        'libdesock/src/connect.c',
        'libdesock/src/desock.c',
        'libdesock/src/dup.c',
        'libdesock/src/epoll.c',
        'libdesock/src/getpeername.c',
        'libdesock/src/getsockname.c',
        'libdesock/src/listen.c',
        'libdesock/src/peekbuffer.c',
        'libdesock/src/poll.c',
        'libdesock/src/read.c',
        'libdesock/src/select.c',
        'libdesock/src/sendfile.c',
        'libdesock/src/shutdown.c',
        'libdesock/src/socket.c',
        'libdesock/src/sockopt.c',
        'libdesock/src/syscall.c',
        'libdesock/src/write.c',
    ],
    include_directories : [
        include_directories('libdesock/include'),
        include_directories('libdesock/include/arch/' + get_option('arch'))
    ],
    c_args : args,
    link_args : '-Wl,-e,desock_main',
    install : false,
    dependencies : [
        dependency('threads')
    ]
)

static_library('desock', 
    sources : [
        'libdesock/src/accept.c',
        'libdesock/src/bind.c',
        'libdesock/src/close.c',
        'libdesock/src/connect.c',
        'libdesock/src/desock.c',
        'libdesock/src/dup.c',
        'libdesock/src/epoll.c',
        'libdesock/src/getpeername.c',
        'libdesock/src/getsockname.c',
        'libdesock/src/listen.c',
        'libdesock/src/peekbuffer.c',
        'libdesock/src/poll.c',
        'libdesock/src/read.c',
        'libdesock/src/select.c',
        'libdesock/src/sendfile.c',
        'libdesock/src/shutdown.c',
        'libdesock/src/socket.c',
        'libdesock/src/sockopt.c',
        'libdesock/src/syscall.c',
        'libdesock/src/write.c',
    ],
    include_directories : [
        include_directories('libdesock/include'),
        include_directories('libdesock/include/arch/' + get_option('arch'))
    ],
    c_args : args,
    link_args : '-Wl,-e,desock_main',
    install : false,
    dependencies : [
        dependency('threads')
    ]
)

exported_functions = [
    'accept', 
    'accept4', 
    'bind', 
    'connect', 
    'close',
    'dup',
    'dup2',
    'dup3',
    'epoll_create',
    'epoll_create1',
    'epoll_ctl',
    'epoll_pwait',
    'epoll_wait',
    'epoll_pwait2',
    'getpeername',
    'getsockname',
    'listen',
    'poll',
    'ppoll',
    'read',
    'recvfrom',
    'recv',
    'recvmsg',
    'recvmmsg',
    'readv',
    'select',
    'pselect',
    'sendfile',
    'sendfile64',
    'shutdown',
    'socket',
    'setsockopt',
    'write',
    'send',
    'sendto',
    'sendmsg',
    'sendmmsg',
    'writev'
]

strip_cmd = ['strip']

foreach fun : exported_functions
    strip_cmd += '--keep-symbol=@0@'.format(fun)
endforeach

strip_cmd += '@INPUT@'

custom_target('target', 
    depends : libdesock_shared,
    input : libdesock_shared,
    output : 'fake',
    command : strip_cmd,
    build_by_default : true
)
