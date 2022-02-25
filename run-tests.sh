#!/bin/bash

set -e;

pytest="pytest -q -x -s"
dir=$(mktemp -d)

meson setup "$dir"

pushd "$dir"
meson configure -D b_coverage=true -D debug_desock=true -D desock_server=true -D desock_client=true -D fd_table_size=512
meson compile
popd

export LIBDESOCK=$(realpath "$dir/libdesock.so")

$pytest tests/test_read.py tests/test_write.py tests/test_peekbuffer.py tests/test_sendfile.py
$pytest tests/test_accept.py
$pytest tests/test_bind.py
$pytest tests/test_dup.py
$pytest tests/test_standard.py
$pytest tests/test_select.py
$pytest tests/test_poll.py
$pytest tests/test_epoll.py
$pytest tests/test_metadata.py

pushd "$dir"
find libdesock.so.p | grep -E '\.o$' | xargs gcov
popd

mv "$dir/"*.gcov tests/coverage

rm -rf "$dir"

cd tests
echo
echo '########################'
./get-coverage.py
echo '########################'
