# Tests

First, build libdesock in `../build` with:
```
meson configure -D b_coverage=true -D debug_desock=true -D desock_client=true -D desock_server=true -D multiple_requests=true -D request_delimiter="---" -D allow_dup_stdin=true
```
 
Then, build the tests:
```
make
```

And finally, run the tests:
```
./run.sh
```
or
```
LD_PRELOAD=../build/libdesock.so <TEST>
```

All tests should exit with status code 0.

## Coverage Report
`run.sh` automatically generates a coverage report in `./report`.
