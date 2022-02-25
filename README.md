# De-socketing for Fuzzing

When fuzzing network applications the fuzzers provide their input via stdin
although the applications get their input over network connections.
This library redirects all network communication to stdin and stdout such that 
network applications can be traditionally fuzzed with AFL++.

For an in-depth explanation of de-socketing see our [blog post](https://lolcads.github.io/posts/2022/02/libdesock/).

## Building
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

| Option          | Description                                                                                | Default |
|-----------------|--------------------------------------------------------------------------------------------|---------|
| `arch`          | The CPU architecture for which you are compiling libdesock.so                              | x86_64  |
| `debug_desock`  | If this is true, calls to functions in libdesock.so get logged to stderr.                  | false   |
| `desock_client` | If this is true, calls to `connect()` get hooked. This enables the desocketing of clients. | false   |
| `desock_server` | If this is true, calls to `bind()` get hooked. This enables the desocketing of servers.    | true    |
| `fd_table_size` | Only fds < `fd_table_size` can be desocked.                                                | 128     |
| `interpreter`   | Path to ld.so (will be determined dynamically if not set)                                  |         |

If configuration is done compile with
```sh
meson compile
```

This creates a shared library `./build/libdesock.so` and a static library `./build/libdesock.a`.

## Usage
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

If you are using libdesock and AFL for fuzzing, the programs under test
usually require a special setup to work with AFL. Checkout our [examples](./examples) 
directory for some examples on how to properly setup network applications for fuzzing.

## Known Bugs
- TCP servers using [libuv](https://libuv.org/) cannot be de-socket-ed (yet). De-socketing of libuv currently only works with UDP servers. It only takes a small change to fix this though, if anyone needs this create an issue.
- `ioctl()` is not supported. Make sure your target does not rely on `ioctl` requests

## System Call Emulation

System call emulation is partly done using musl libc code (https://musl.libc.org/) - see `libdesock/include`
