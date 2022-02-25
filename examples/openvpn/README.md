# Fuzzing OpenVPN with AFL

This document explains how to setup [OpenVPN 2.5.5](https://github.com/OpenVPN/openvpn) for fuzzing
with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://github.com/OpenVPN/openvpn
cd openvpn
git checkout v2.5.5
```

### Patching the source
In `src/openvpn/mtcp.c:820` replace
```diff
     /* per-packet event loop */
-    while (true)
+    for (int i = 0; i < 2; ++i)
     {
```

### Build configuration
```sh
autoreconf -i
CC=afl-clang-fast CFLAGS="-fsanitize=address -g -Og" LDFLAGS="-fsanitize=address" ./configure --disable-lzo --disable-lz4 --enable-comp-stub --disable-unit-tests
make
```

### Runtime Configuration
Create `fuzz.conf`:
```
local 127.0.0.1
port 1194
proto tcp
dev tun
server 10.8.0.0 255.255.255.0
cipher AES-128-GCM
verb 0

ca ca.crt
cert server.crt
key server.key
dh dh2048.pem
```

Copy the necessary keys:
```sh
cp sample/sample-keys/{ca.crt,server.crt,server.key,dh2048.pem} .
```

### Fuzzing Setup
```
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- src/openvpn/openvpn fuzz.conf
```
