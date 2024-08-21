ALL_TESTS=test_accept

all: $(ALL_TESTS)

test_%: test_%.c main.c
	gcc -Wall -Wextra -Wpedantic -Werror -o $@ -I. $^ -g

.PHONY: clean
clean:
	@rm -vf $(ALL_TESTS)
