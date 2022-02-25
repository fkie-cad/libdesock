# Fuzzing cupsd with AFL

This document explains how to setup [cupsd 2.3.3](https://github.com/OpenPrinting/cups) for fuzzing
with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://github.com/OpenPrinting/cups
cd cups
git checkout v2.3.3op2
```

### Patching the source
In `scheduler/main.c` add
```diff
@@ -718,6 +718,7 @@ main(int  argc,				/* I - Number of command-line args */
   report_time   = 0;
   senddoc_time  = current_time;
 
+  int __custom_events = 0;
   while (!stop_scheduler)
   {
    /*
@@ -1147,6 +1148,8 @@ main(int  argc,				/* I - Number of command-line args */
       LastEvent  = CUPSD_EVENT_NONE;
       event_time = current_time;
     }
+    
+    if (++__custom_events >= 2) exit(0);
   }
```

### Build configuration
```sh
mkdir install
CC=afl-clang-fast CXX=afl-clang-fast++ CFLAGS="-fsanitize=address -g -Og" LDFLAGS="-fsanitize=address" \
./configure --prefix=$PWD/install \
           --with-menudir=$PWD/install \
           --with-icondir=$PWD/install  \
           --enable-debug \
           --without-pam-module \
           --with-cups-user=$USER \
           --with-cups-group=$USER \
           --without-xinetd \
           --disable-systemd \
           --disable-threads \
           --disable-dbus \
           --without-rcdir
```

In `Makedefs` set the linker to `afl-clang-fast++`:
```
LD		=	afl-clang-fast++
```

Then build the program
```
make install
```

### Runtime Configuration
Create `fuzz.conf`:
```
LogLevel error
Listen *:6666
Timeout 10
ErrorPolicy abort-job
HostNameLookups Off
```

### Fuzzing Setup
```
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- ./sbin/cupsd -f -c fuzz.conf
```
