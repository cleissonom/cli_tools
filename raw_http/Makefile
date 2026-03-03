CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -Wpedantic -std=gnu11
LDLIBS = -lcurl
SRC = src/main.c src/rh_cli.c src/rh_parse.c src/rh_headers.c src/rh_response.c src/rh_client.c src/rh_output.c
TEST_SRC = tests/test_parse.c src/rh_parse.c src/rh_client.c src/rh_headers.c src/rh_response.c

build:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o raw_http $(SRC) $(LDLIBS)

test:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_parse $(TEST_SRC) $(LDLIBS)
	@./test_parse

clean:
	@rm -f raw_http test_parse
