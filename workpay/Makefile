CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LDLIBS = -lcurl -lm
SRC = src/main.c src/sc_parse.c src/sc_money.c src/sc_cli.c src/sc_quota.c src/sc_api.c src/sc_config.c

build:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o workpay $(SRC) $(LDLIBS)
