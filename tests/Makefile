ALL_TESTS=test_accept test_epoll test_poll test_select test_multi test_threads

all: $(ALL_TESTS)

test_%: test_%.c main.c
	cc -Wall -Wextra -Wpedantic -Werror -O0 -o $@ -I. -I../src/include $^ -ldesock -L../build -pthread -g

.PHONY: clean
clean:
	@rm -vf $(ALL_TESTS)
