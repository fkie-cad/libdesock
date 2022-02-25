# Fuzzing vsftpd with AFL

This document explains how to setup [vsftpd 3.0.5](https://security.appspot.com/vsftpd.html)
for fuzzing with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
wget https://security.appspot.com/downloads/vsftpd-3.0.5.tar.gz
tar -xf vsftpd-3.0.5.tar.gz
cd vsftpd-3.0.5
```

### Patching the source
First we must prohibit the creation of any child processes.
To do that we insert in `standalone.c:156`:
```diff
@@ -153,6 +153,7 @@ vsf_standalone_main(void)
     child_info.num_this_ip = 0;
     p_raw_addr = vsf_sysutil_sockaddr_get_raw_addr(p_accept_addr);
     child_info.num_this_ip = handle_ip_count(p_raw_addr);
+    /*
     if (tunable_isolate)
     {
       if (tunable_http_enable && tunable_isolate_network)
@@ -168,6 +169,8 @@ vsf_standalone_main(void)
     {
       new_child = vsf_sysutil_fork_failok();
     }
+    */
+    new_child = 0;
     if (new_child != 0)
     {
       /* Parent context */
```

vsftpd duplicates the FTP command socket to stdin, stdout and stderr. This
interferes with the desocketing library so we prohibit that too.
In `defs.h:6` replace
```diff
@@ -3,7 +3,7 @@
 
 #define VSFTP_DEFAULT_CONFIG    "/etc/vsftpd.conf"
 
-#define VSFTP_COMMAND_FD        0
+#define VSFTP_COMMAND_FD        29
 
 #define VSFTP_PASSWORD_MAX      128
 #define VSFTP_USERNAME_MAX      128
```
and in `standalone.c:208` replace
```diff
@@ -205,9 +205,7 @@ static void
 prepare_child(int new_client_sock)
 {
   /* We must satisfy the contract: command socket on fd 0, 1, 2 */
-  vsf_sysutil_dupfd2(new_client_sock, 0);
-  vsf_sysutil_dupfd2(new_client_sock, 1);
-  vsf_sysutil_dupfd2(new_client_sock, 2);
+  vsf_sysutil_dupfd2(new_client_sock, VSFTP_COMMAND_FD);
   if (new_client_sock > 2)
   {
     vsf_sysutil_close(new_client_sock);
```

Next vsftpd enforces a custom memory limit. This interferes with ASAN
so we insert in `sysutil.c:2795`
```diff
@@ -2793,6 +2793,7 @@ void
 vsf_sysutil_set_address_space_limit(unsigned long bytes)
 {
   /* Unfortunately, OpenBSD is missing RLIMIT_AS. */
+  return;
 #ifdef RLIMIT_AS
   int ret;
   struct rlimit rlim;
```

Then we add a deferred forkserver to vsftpd by inserting
in `prelogin.c:61`
```diff
@@ -59,6 +59,7 @@ init_connection(struct vsf_session* p_sess)
   {
     emit_greeting(p_sess);
   }
+  __AFL_INIT();
   parse_username_password(p_sess);
 }
```

We also want to prevent vsftpd from interfering with the forkserver
so we disable its SIGCHLD handler in `standalone.c:77`
```diff
@@ -74,7 +74,7 @@ vsf_standalone_main(void)
   {
     vsf_sysutil_setproctitle("LISTENER");
   }
-  vsf_sysutil_install_sighandler(kVSFSysUtilSigCHLD, handle_sigchld, 0, 1);
+  //vsf_sysutil_install_sighandler(kVSFSysUtilSigCHLD, handle_sigchld, 0, 1);
   vsf_sysutil_install_sighandler(kVSFSysUtilSigHUP, handle_sighup, 0, 1);
   if (tunable_listen)
   {
```

Lastly to prevent endless loops we disable the `bug()` function
in `utility.c:43`
```diff
@@ -40,6 +40,7 @@ die2(const char* p_text1, const char* p_text2)
 void
 bug(const char* p_text)
 {
+  return;
   /* Rats. Try and write the reason to the network for diagnostics */
   vsf_sysutil_activate_noblock(VSFTP_COMMAND_FD);
   (void) vsf_sysutil_write_loop(VSFTP_COMMAND_FD, "500 OOPS: ", 10);
```

### Build configuration
In `Makefile` replace
```diff
@@ -1,16 +1,16 @@
 # Makefile for systems with GNU tools
-CC 	=	gcc
+CC 	=	afl-clang-fast
 INSTALL	=	install
 IFLAGS  = -idirafter dummyinc
 #CFLAGS = -g
-CFLAGS	=	-O2 -fPIE -fstack-protector --param=ssp-buffer-size=4 \
-	-Wall -W -Wshadow -Werror -Wformat-security \
+CFLAGS	=	-fsanitize=address -g -Og -fPIE -fstack-protector \
+	-Wall -W -Wshadow -Wformat-security \
 	-D_FORTIFY_SOURCE=2 \
 	#-pedantic -Wconversion
 
 LIBS	=	`./vsf_findlibs.sh`
-LINK	=	-Wl,-s
-LDFLAGS	=	-fPIE -pie -Wl,-z,relro -Wl,-z,now
+LINK	=	
+LDFLAGS	=	-fPIE -pie -Wl,-z,relro -Wl,-z,now -fsanitize=address
 
 OBJS	=	main.o utility.o prelogin.o ftpcmdio.o postlogin.o privsock.o \
 		tunables.o ftpdataio.o secbuf.o ls.o \
```
    
and execute
```sh
make
```

### Runtime configuration
Save the following configuration as `fuzz.conf`.
```
listen=YES
seccomp_sandbox=NO
one_process_model=YES

# User management
anonymous_enable=YES
no_anon_password=YES
nopriv_user=nobody

# Permissions
connect_from_port_20=NO
run_as_launching_user=YES
listen_port=2121
listen_address=127.0.0.1
pasv_address=127.0.0.1

# Filesystem interactions
write_enable=NO
download_enable=NO
```

### Fuzzing Setup
Now it's time to start AFL:
```
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- ./vsftpd fuzz.conf
```
