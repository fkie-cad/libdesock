ALL_TESTS=test_accept test_epoll

all: $(ALL_TESTS)

test_%: test_%.c main.c
	gcc -Wall -Wextra -Wpedantic -Werror -o $@ -I. -I../src/include $^ -ldesock -L../build -g

.PHONY: clean
clean:
	@rm -vf $(ALL_TESTS)
