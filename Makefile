all:
	clang -g -Wall -Werror -std=c99 libserg_test.c -o test -lpthread
