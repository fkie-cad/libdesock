project('libdesock', 'c', license: 'MIT', version: '1.1')

if get_option('interpreter') == ''
    r = run_command('sh', '-c', 'readelf -l /bin/ls | grep "program interpreter" | cut -d: -f2  | sed -e "s/^.//" -e "s/.$//"', check: true)
    interpreter = r.stdout().strip()
else
    interpreter = get_option('interpreter')
endif

args = [
    '-Ofast',
    '-march=native',
    '-fomit-frame-pointer',
    '-fno-stack-protector',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
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

if get_option('allow_dup_stdin')
    args += '-D DUP_STDIN'
endif

if get_option('multiple_requests')
    args += '-D MULTI_REQUEST'
endif

args += '-D REQUEST_DELIMITER="@0@"'.format(get_option('request_delimiter'))

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
    'libdesock/src/hooks.c',
    'libdesock/src/multi.c',
]
include_directories = [
    include_directories('libdesock/include'),
    include_directories('libdesock/include/arch/' + get_option('arch'))
]

shared_library('desock', 
    sources : sources,
    include_directories : include_directories,
    c_args : args + ['-flto'],
    link_args : ['-Wl,-e,desock_main', '-flto'],
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
