# Fuzzing redis with AFL

This document explains how to setup [redis 6.2.6](https://github.com/redis/redis/releases/tag/6.2.6) for fuzzing
with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://github.com/redis/redis
cd redis
git checkout 6.2.6
```

### Patching the source
In `src/networking.c` replace ...
```diff
@@ -1047,7 +1047,7 @@ void clientAcceptHandler(connection *conn) {
                           c);
 }
 
-#define MAX_ACCEPTS_PER_CALL 1000
+#define MAX_ACCEPTS_PER_CALL 1
 static void acceptCommonHandler(connection *conn, int flags, char *ip) {
     client *c;
     char conninfo[100];
```
... and insert
```diff
@@ -2197,6 +2197,7 @@ void readQueryFromClient(connection *conn) {
     } else if (nread == 0) {
         serverLog(LL_VERBOSE, "Client closed connection");
         freeClientAsync(c);
+        exit(0);
         return;
     } else if (c->flags & CLIENT_MASTER) {
         /* Append the query buffer to the pending (not applied) buffer
```

### Build configuration
```sh
mkdir install
make CC="afl-clang-fast" REDIS_CFLAGS="-fsanitize=address -g -Og" REDIS_LDFLAGS="-fsanitize=address" MALLOC="libc"
make PREFIX=$PWD/install install
```

### Runtime configuration
Create `fuzz.conf`:
```
bind 127.0.0.1
daemonize no
supervised no
loglevel warning
logfile /dev/null
always-show-logo no
io-threads 1
timeout 0
databases 1
```

### Fuzzing Setup
```
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- ./install/bin/redis-server fuzz.conf
```
