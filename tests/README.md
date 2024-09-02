# Tests

First, build libdesock with:
```
meson configure -D debug_desock=true -D desock_client=true -D desock_server=true -D multiple_requests=true -D request_delimiter="---"
```
 
Then, build the tests:
```
make
```

And finally, run the tests:
```
LD_PRELOAD=../build/libdesock.so <TEST>
```

All tests should exit with status code 0.
