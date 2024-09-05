#!/bin/bash
set -e

export LD_PRELOAD=../build/libdesock.so

# Connection management
./test_accept
./test_threads

# I/O multiplexing functions
./test_epoll
./test_select
./test_poll

# Input management
./test_multi

unset LD_PRELOAD
echo
echo '##################'
echo ' ALL TESTS PASSED'
echo '##################'

pushd ../build
find ./libdesock.so.p | grep '\.c\.o$' | xargs gcov
popd

mkdir -p report
mv ../build/*.gcov ./report

