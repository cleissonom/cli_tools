CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LDLIBS = -lcurl -lm
SRC = src/main.c src/salary_calculator.c src/sc_parse.c src/sc_money.c src/sc_cli.c

build:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o salary_calculator $(SRC) $(LDLIBS)
