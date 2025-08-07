# libdesock

Network applications are hard to fuzz with traditional fuzzers because they a) 
expect their input over network connections, and b) process multiple inputs (multiple packets) in a single run.   

libdesock solves this problem by a) redirecting network I/O to stdin and stdout, and b) treating an input
as a sequence of chunks that get individually fed to the application.

It functions as a library that when preloaded with `LD_PRELOAD` replaces the network stack of the
libc with its own stack that emulates all network operations in user-space.   
This has multiple advantages for fuzzing:

1. It reduces the syscalls of the target
2. It automatically synchronizes multi-threaded programs
3. No extra harnessing is needed to get the fuzz input to the application

libdesock also let's you customize what happens when an application requests data over a network connection.
The default behavior is to read from stdin but this can be changed inside the "input hook" (see `src/hooks.c`).

## How to use
Prepend
```sh
LD_PRELOAD=libdesock.so
```
to the invocation of any network application or
set the environment variable
```sh
AFL_PRELOAD=libdesock.so
```
when using AFL++.

Here is an example how to desock nginx:   
![](./demo.svg)

## How to build
Libdesock uses `meson` as its build system.

```sh
meson setup ./build
cd ./build
```

You can configure the build using
```sh
meson configure -D <optname>=<optvalue>
```

You can get an overview over all options with
```sh
meson configure
```

The following options are specific to libdesock:

| Option           | Description                                                                                | Default |
|------------------|--------------------------------------------------------------------------------------------|---------|
| `arch`           | The CPU architecture for which you are compiling libdesock.so                              | x86_64  |
| `multiple_requests`| If this is true, a delimiter will be used to return different data from subsequent read calls     | false   |
| `request_delimiter` | The delimiter that separates multiple requests | `-=^..^=-` |
| `desock_client`  | If this is true, calls to `connect()` get hooked. This enables the desocketing of clients | false   |
| `desock_server`  | If this is true, calls to `bind()` get hooked. This enables the desocketing of servers    | true    |
| `allow_dup_stdin`| If this is true, calls to `dup*()` are allowed for stdin. This enables stdin redirection when running under gdb    | false   |
| `debug_desock`   | If this is true, calls to functions in libdesock.so get logged to stderr                  | false   |
| `fd_table_size`  | Only fds < `fd_table_size` can be desocketed                                                | 128     |
| `interpreter`    | Path to ld.so (will be determined dynamically if not set)                                  |         |
| `symbol_version` | If this is set, every exported symbol has this version |  |
| `max_conns` | The number of simulatenous connections that can be desocketed. A value other than 1 doesn't make sense at the moment | 1 |

If configuration is done compile with
```sh
meson compile
```

This creates a shared library `build/libdesock.so` and a static library `build/libdesock.a`.

## How to fuzz
If you are using libdesock and AFL++ for fuzzing, the programs under test
usually require a special setup to work with AFL++. Check out our [examples](./examples) 
directory for some examples on how to setup network applications for fuzzing.

Additionally, you can also read up on how libdesock was used to fuzz...
- [µC/OS by Cisco Talos](https://blog.talosintelligence.com/fuzzing-uc-os-protocol-stacks/)
- [Exim by us](https://github.com/pd-fkie/exim-fuzzer)

## Known bugs
- TCP servers using [libuv](https://libuv.org/) cannot be desocketed (yet). Desocketing of libuv currently only works with UDP servers. It only takes a small change to fix this though, if you need this, create an issue.
- `ioctl()` is not supported. Make sure your target does not rely on `ioctl` requests
