# Fuzzing OpenSSH with AFL

This document explains how to setup [openssh 8.8](https://www.openssh.com/) for fuzzing
with AFL and libdesock.

__Disclaimer:__ This document only shows the modifications necessary to get the desocketing working.
Further modifications may be necessary for successful fuzzing.

### Installing the source
```sh
git clone https://github.com/openssh/openssh-portable
cd openssh-portable
git checkout V_8_8_P1
```

### Patching the source
Disable privilege separation in `sshd.c:234`:
```diff
@@ -231,7 +231,7 @@ static int *startup_flags = NULL;	/* Indicates child closed listener */
 static int startup_pipe = -1;		/* in child */
 
 /* variables used for privilege separation */
-int use_privsep = -1;
+int use_privsep = PRIVSEP_OFF;
 struct monitor *pmonitor = NULL;
 int privsep_is_preauth = 1;
 static int privsep_chroot = 1;
```

Disable PRNG in `openbsd-compat/arc4random.c:244` by inserting:
```diff
@@ -241,6 +241,7 @@ arc4random_addrandom(u_char *dat, int datlen)
 u_int32_t
 arc4random(void)
 {
+    return 0;
 	u_int32_t val;
 
 	_ARC4_LOCK();
```

In `cipher.c` replace
```diff
@@ -75,7 +75,7 @@ struct sshcipher {
 #define CFLAG_CHACHAPOLY	(1<<1)
 #define CFLAG_AESCTR		(1<<2)
 #define CFLAG_NONE		(1<<3)
-#define CFLAG_INTERNAL		CFLAG_NONE /* Don't use "none" for packets */
+#define CFLAG_INTERNAL		0 /* Don't use "none" for packets */
 #ifdef WITH_OPENSSL
 	const EVP_CIPHER	*(*evptype)(void);
 #else
```

In `openbsd-compat/bsd-closefrom.c:139` insert:
```diff
@@ -136,7 +136,8 @@ closefrom(int lowfd)
 	while ((dent = readdir(dirp)) != NULL) {
 	    fd = strtol(dent->d_name, &endp, 10);
 	    if (dent->d_name != endp && *endp == '\0' &&
-		fd >= 0 && fd < INT_MAX && fd >= lowfd && fd != dirfd(dirp))
+		fd >= 0 && fd < INT_MAX && fd >= lowfd && fd != dirfd(dirp)
+        && fd < 32)
 		(void) close((int) fd);
 	}
 	(void) closedir(dirp);
```

### Build configuration
```sh
autoreconf -i
CC=afl-clang-fast LIBS=-ldl CFLAGS="-fsanitize=address -g -Og" LDFLAGS="-fsanitize=address" ./configure \
    --disable-strip \
    --without-zlib \
    --without-pam \
    --without-selinux \
    --with-privsep-path=empty \
    --with-sandbox=no \
    --with-privsep-user=$USER
make sshd
```

### Runtime configuration
Generate a host key:
```
ssh-keygen -f key -N ""
```

Save the following configuration in `fuzz.conf`:
```
HostKey <FULL PATH TO>/key
AuthenticationMethods none
Banner none
Compression no
LogLevel QUIET
PermitRootLogin no
PermitTTY no
PermitUserRC no
PrintMotd no
UsePrivilegeSeparation no
Ciphers none
```

### Fuzzing Setup
```
export AFL_PRELOAD=libdesock.so
export AFL_TMPDIR=/tmp
afl-fuzz -i corpus -o findings -m none -- $(realpath ./sshd) -4 -E /dev/null -D -f fuzz.conf -p 12222 -r -d
```
