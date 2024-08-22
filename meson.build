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
link_args = [
    '-Wl,-e,__libdesock_main',
    '-flto'
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

if get_option('symbol_version') != ''
    version_file = '@0@ {};'.format(get_option('symbol_version'))
    file_path = '@0@/syms.map'.format(meson.build_root())
    run_command('sh', '-c', 'echo "@0@" > @1@'.format(version_file, file_path), check: true)
    
    args += '-D VERSION_NAME="@0@"'.format(get_option('symbol_version'))
    link_args += '-Wl,--version-script=@0@'.format(file_path)
endif

args += '-D REQUEST_DELIMITER="@0@"'.format(get_option('request_delimiter'))

sources = [
    'src/main.c',
    'src/desock.c',
    'src/stub_sockaddr.c',
    'src/syscall.c',
    'src/accept.c',
    'src/bind.c',
    'src/connect.c',
    'src/socket.c',
    'src/listen.c',
    'src/close.c',
    'src/test_helper.c',
]
include_directories = [
    include_directories('src/include'),
    include_directories('src/include/arch/' + get_option('arch'))
]

shared_library('desock', 
    sources : sources,
    include_directories : include_directories,
    c_args : args + ['-D SHARED', '-flto'],
    link_args : link_args,
    install : false,
    dependencies : [
        dependency('threads')
    ]
)

static_library('desock', 
    sources : sources,
    include_directories : include_directories,
    c_args : args,
    install : false,
    dependencies : [
        dependency('threads')
    ]
)
