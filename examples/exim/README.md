# Fuzzing Exim with AFL

This document explains how to setup [exim 4.95](https://github.com/exim/exim)
for fuzzing with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://github.com/exim/exim
cd exim
git checkout exim-4.95
```

### Patching the source
We need exim to exit after processing one request. We can do that by calling `exit()`
in the appropriate places.    
In `src/src/daemon.c:918` insert
```diff
@@ -916,6 +916,7 @@ while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
           smtp_accept_count, (smtp_accept_count == 1)? "" : "es");
         break;
         }
+    if (!smtp_accept_count) _exit(0);
     if (i < smtp_accept_max) continue;  /* Found an accepting process */
     }
```
In `src/src/deliver.c:2406` insert
```diff
@@ -2403,6 +2403,8 @@ if ((pid = exim_fork(US"delivery-local")) == 0)
   file_format in appendfile. */
 
   PASS_BACK:
+  search_tidyup();
+  _exit(0);
 
```
In `src/src/smtp_in.c:5751` insert
```diff
@@ -5748,6 +5748,7 @@ while (done <= 0)
       incomplete_transaction_log(US"connection lost");
       smtp_notquit_exit(US"connection-lost", US"421",
 	US"%s lost input connection", smtp_active_hostname);
+      _exit(0);
 
```

Next we need to prohibit the creation of child processes. We do
that by simply returning `0` from `fork()`.    
In `src/src/functions.h:1222` change
```diff
@@ -1219,7 +1219,7 @@ exim_fork(const unsigned char * purpose)
 {
 pid_t pid;
 DEBUG(D_any) debug_printf("%s forking for %s\n", process_purpose, purpose);
-if ((pid = fork()) == 0)
+if ((pid = 0) == 0)
   {
   process_purpose = purpose;
   DEBUG(D_any) debug_printf("postfork: %s\n", purpose);
```

Then we need to setup exim for correct processing of AFLs inputs.
Exim expects small delays between all SMTP messages. This of course
hinders fuzzing because AFLs input is a flat buffer where all messages
are already available.    
In `src/src/globals.c:411` replace
```diff
@@ -408,7 +408,7 @@ BOOL    sender_helo_dnssec     = FALSE;
 BOOL    sender_host_dnssec     = FALSE;
 BOOL    smtp_accept_keepalive  = TRUE;
 BOOL    smtp_check_spool_space = TRUE;
-BOOL    smtp_enforce_sync      = TRUE;
+BOOL    smtp_enforce_sync      = FALSE;
 BOOL    smtp_etrn_serialize    = TRUE;
 BOOL    smtp_input             = FALSE;
 BOOL    smtp_return_error_details = FALSE;
```
In `src/src/smtp_in.c:351` insert
```diff
@@ -348,7 +348,7 @@ wouldblock_reading(void)
 int fd, rc;
 fd_set fds;
 struct timeval tzero = {.tv_sec = 0, .tv_usec = 0};
-
+return TRUE;
 #ifndef DISABLE_TLS
 if (tls_in.active.sock >= 0)
  return !tls_could_read();
```

For performance reasons we add a deferred forkserver to exim
by inserting in `src/src/exim.c:4946`
```diff
@@ -4943,7 +4943,7 @@ if (f.daemon_listen || f.inetd_wait_mode || queue_interval > 0)
 # endif
     }
 #endif
-
+  __AFL_INIT();
   daemon_go();
   }
```

Lastly we need to disable exims custom handling of segfaults by replacing in
`src/src/exim.c:1822`
```diff
@@ -1819,7 +1819,7 @@ descriptive text. */
 process_info = store_get(PROCESS_INFO_SIZE, TRUE);	/* tainted */
 set_process_info("initializing");
 os_restarting_signal(SIGUSR1, usr1_handler);		/* exiwhat */
-signal(SIGSEGV, segv_handler);				/* log faults */
+//signal(SIGSEGV, segv_handler);				/* log faults */
 
 /* If running in a dockerized environment, the TERM signal is only
 delegated to the PID 1 if we request it by setting an signal handler */
```

### Build configuration
Exim keeps all build configuration in a single file called `src/Local/Makefile`.

```sh
mkdir src/Local
```

Create the `Makefile` with the following contents:
```make
# The following lines have to be adjusted to your system
BIN_DIRECTORY=$(PWD)/src/build-Linux-x86_64
CONFIGURE_FILE=$(PWD)/fuzz.conf
EXIM_USER=$(USER)
EXIM_GROUP=$(USER)
CONFIGURE_OWNER=$(USER)
CONFIGURE_GROUP=$(USER)
LOG_FILE_PATH=$(PWD)/log/%s
SYSTEM_ALIASES_FILE=$(PWD)/etc/aliases

# The following can be used as is
CC=afl-clang-fast
LNCC=afl-clang-fast
LFLAGS=-fsanitize=address
CFLAGS=-fsanitize=address -g -Og
SPOOL_DIRECTORY=/tmp/exim-spool
DISABLE_TLS=yes
ROUTER_ACCEPT=yes
ROUTER_DNSLOOKUP=yes
ROUTER_IPLITERAL=yes
ROUTER_MANUALROUTE=yes
ROUTER_QUERYPROGRAM=yes
ROUTER_REDIRECT=yes
TRANSPORT_APPENDFILE=yes
TRANSPORT_AUTOREPLY=yes
TRANSPORT_PIPE=yes
TRANSPORT_SMTP=yes
SUPPORT_MAILDIR=yes
SUPPORT_MAILSTORE=yes
SUPPORT_MBX=yes
LOOKUP_PASSWD=yes
PCRE_CONFIG=yes
DISABLE_MAL_AVE=yes
DISABLE_MAL_KAV=yes
DISABLE_MAL_MKS=yes
DISABLE_DNSSEC=yes
DISABLE_EVENT=yes
DISABLE_PIPE_CONNECT=yes
FIXED_NEVER_USERS=root
ALLOW_INSECURE_TAINTED_DATA=yes
AUTH_PLAINTEXT=yes
HEADERS_CHARSET="ISO-8859-1"
EXICYCLOG_MAX=10
COMPRESS_COMMAND=/usr/bin/gzip
COMPRESS_SUFFIX=gz
ZCAT_COMMAND=/usr/bin/zcat
NO_SYMLINK=yes
EXIM_TMPDIR="/tmp"
DELIVER_IN_BUFFER_SIZE=4096
DELIVER_OUT_BUFFER_SIZE=4096
NVALGRIND=1
PID_FILE_PATH=/dev/null
```

You must replace `$(USER)` with your local username and `$(PWD)` with the
directory of the exim repo.

Then build exim:
```sh
make -C src
```

If you get the message
```
>>> exim binary built
```
everything went well.

### Runtime configuration
Exim looks for a configuration file `fuzz.conf` in the exim repo root.
Create it with the following contents:
```
no_message_logs
log_selector = -all
primary_hostname = localhost
domainlist local_domains = @
domainlist relay_to_domains = localhost
hostlist   relay_from_hosts = localhost

acl_smtp_rcpt =         accept_everything
.ifdef _HAVE_PRDR
acl_smtp_data_prdr =    accept_everything
.endif
acl_smtp_auth = accept_everything
acl_smtp_connect = accept_everything
acl_smtp_data = accept_everything
acl_smtp_etrn = accept_everything
acl_smtp_expn = accept_everything
acl_smtp_helo = accept_everything
acl_smtp_mail = accept_everything
acl_smtp_mailauth = accept_everything
acl_smtp_predata =accept_everything
acl_smtp_quit = accept_everything
acl_smtp_vrfy = accept_everything

acl_not_smtp = deny_everything
acl_not_smtp_start = deny_everything

daemon_smtp_ports = 2525

never_users = root

dns_dnssec_ok = 0

ignore_bounce_errors_after = 2d

timeout_frozen_after = 7d

begin acl
accept_everything:
  accept
deny_everything:
  deny

begin routers
localuser:
  driver = accept
  check_local_user
  transport = local_delivery
  cannot_route_message = Unknown user

begin transports
local_delivery:
  driver = appendfile
  file = /tmp/mail/$local_part_data
  delivery_date_add
  envelope_to_add
  return_path_add
  group = $USER

begin retry
# Address or Domain    Error       Retries
# -----------------    -----       -------
*                      *           F,2h,15m; G,16h,1h,1.5; F,4d,6h

begin rewrite

begin authenticators
PLAIN:
driver                     = plaintext
public_name = PLAIN
server_set_id              = $auth2
server_prompts             = :

# AUTH credentials are test:test (can be changed here)
server_condition           = ${if and {{eq{$auth2}{test}}{eq{$auth3}{test}}}}
```

Change `group = $USER` to your local username.
This configures exim with only plaintext authentication, no DNSSEC, no TLS and no real ACLs.

### Fuzzing Setup
Now it's time to start AFL:
```sh
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- ./src/build-Linux-x86_64/exim -bdf
```
