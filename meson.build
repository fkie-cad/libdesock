project('libdesock', 'c', license: 'MIT', version: '1.1')

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
    '-Wno-unused-parameter',
    '-Wno-sign-compare',
    '-fvisibility=hidden',
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

sources = [
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
]
include_directories = [
    include_directories('libdesock/include'),
    include_directories('libdesock/include/arch/' + get_option('arch'))
]

libdesock_shared = shared_library('desock', 
    sources : sources,
    include_directories : include_directories,
    c_args : args,
    link_args : '-Wl,-e,desock_main',
    install : false,
    dependencies : [
        dependency('threads')
    ]
)

static_library('desock', 
    sources : sources,
    include_directories : include_directories,
    c_args : args,
    link_args : '-Wl,-e,desock_main',
    install : false,
    dependencies : [
        dependency('threads')
    ]
)
