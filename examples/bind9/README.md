# Fuzzing bind9 with AFL

This document explains how to setup [bind9 9.17.16](https://gitlab.isc.org/isc-projects/bind9.git) for fuzzing
with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://gitlab.isc.org/isc-projects/bind9.git
cd bind9
git checkout v9_17_16
```

### Patching the source
In `lib/ns/client.c` add
```diff
@@ -11,6 +11,7 @@
 
 #include <inttypes.h>
 #include <stdbool.h>
+#include <unistd.h>
 
 #include <isc/aes.h>
 #include <isc/atomic.h>
@@ -234,6 +235,7 @@ ns_client_endrequest(ns_client_t *client) {
                client->sctx->fuzznotify();
        }
 #endif /* ENABLE_AFL */
+    _exit(0);
 }
```

### Build configuration
```sh
autoreconf -i
mkdir -p build/install
cd build
CC="afl-clang-fast" CFLAGS="-fsanitize=address -g -Og" LDFLAGS="-fsanitize=address" ../configure --prefix=$PWD/install --disable-chroot --with-zlib=no --with-libxml2=no
make install
```

### Runtime configuration
Create `example.com.db`:
```
$TTL    604800
@       IN      SOA     ns.example.com. root.example.com. (
                              1         ; Serial
                         604800         ; Refresh
                          86400         ; Retry
                        2419200         ; Expire
                         604800 )       ; Negative Cache TTL
@       IN      NS      ns.example.com.
ns      IN      A       127.0.0.1
sub     IN      A       1.2.3.4
```

Create `fuzz.conf`:
```
options {
     directory "/tmp";
     allow-query-cache { none; };
     allow-query { any; };
     recursion no;
     listen-on port 5353 { 127.0.0.1; };
};

controls { };

zone "example.com" {
     type primary;
     file "example.com.db";
};
```

### Fuzzing Setup
```
cp example.com.db /tmp
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- ./install/sbin/named -c fuzz.conf -g -n 0 -4
```
