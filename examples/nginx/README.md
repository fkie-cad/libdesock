# Fuzzing nginx with AFL

This document explains how to setup [nginx 1.21.4](https://github.com/nginx/nginx) for fuzzing
with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://github.com/nginx/nginx
cd nginx
git checkout release-1.21.4
```

### Patching the source
## Single request fuzzing
nginx runs in an endless loop but we want it to exit
after processing one request. We locate the event loop
in `src/os/unix/ngx_process_cycle.c:297` and replace
```diff
@@ -294,7 +294,7 @@ ngx_single_process_cycle(ngx_cycle_t *cycle)
         }
     }
 
-    for ( ;; ) {
+    for (i = 0; i < 2; ++i) {
         ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");
 
         ngx_process_events_and_timers(cycle);
```
The first iteration is for the `accept()`, the second iteration is for receiving
the request.

Then we add a deferred forkserver to AFL by inserting in `src/os/unix/ngx_process_cycle.c:296`
```diff
@@ -293,7 +293,7 @@ ngx_single_process_cycle(ngx_cycle_t *cycle)
             }
         }
     }
-
+    __AFL_INIT();
     for (i = 0; i < 2; ++i) {
         ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");
```

## Multiple request fuzzing
nginx runs in an endless loop but we want it to exit
after processing all of the requests in our test file. The connection will be closed when `read` returns zero. We add the line in `src/http/ngx_http_request.c:3787` to indicate that nginx should cleanly exit upon the next iteration of the main processing loop. <mark> If patching for multiple requests do not make the patches for single requests</mark>
```diff
@@ -3785,6 +3785,7 @@ ngx_http_close_connection(ngx_connection_t *c)
     ngx_close_connection(c);
 
     ngx_destroy_pool(pool);
+    ngx_terminate=1;
 }
 ```

### Build configuration
```sh
auto/configure \
  --prefix=$PWD \
  --conf-path=$PWD/fuzz.conf \
  --user=$USER --group=$USER \
  --with-debug \
  --with-cc=afl-clang-fast \
  --with-cc-opt="-fsanitize=address -g -Og -Wno-error=unused-but-set-variable" \
  --with-ld-opt="-fsanitize=address" \
  --pid-path=/dev/null \
  --with-poll_module
```
and
```sh
make
```

### Runtime configuration
We have to tell nginx not to use any child processes / threads
and to not have any disk I/O.
We configure that in `fuzz.conf`.
```
master_process off;
daemon off;
error_log /dev/null;
events {
    worker_connections  1024;
    use poll;
    multi_accept off;
}

http {
    default_type  application/octet-stream;
    sendfile        on;
    keepalive_timeout  65;

    server {
        listen       20801;
        server_name  localhost;
        access_log  /dev/null;

        location / {
            root   html;
            index  index.html;
        }
    }
}
```

### Fuzzing Setup
```
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- ./objs/nginx
```
