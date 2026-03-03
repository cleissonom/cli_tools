CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LDLIBS = -lcurl -lm
SRC = src/main.c src/salary_calculator.c

build:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o salary_calculator $(SRC) $(LDLIBS)
