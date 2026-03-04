CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LDLIBS = -lcurl -lm
SRC = src/main.c src/sc_parse.c src/sc_money.c src/sc_cli.c src/sc_quota.c src/sc_api.c src/sc_config.c
TEST_PARSE_SRC = tests/test_parse.c src/sc_parse.c
TEST_MONEY_SRC = tests/test_money.c src/sc_money.c
TEST_CLI_SRC = tests/test_cli.c src/sc_cli.c src/sc_parse.c
TEST_QUOTA_SRC = tests/test_quota.c src/sc_quota.c src/sc_config.c

build:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o workpay $(SRC) $(LDLIBS)

test:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_parse $(TEST_PARSE_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_money $(TEST_MONEY_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_cli $(TEST_CLI_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_quota $(TEST_QUOTA_SRC)
	@./test_parse
	@./test_money
	@./test_cli
	@./test_quota

clean:
	@rm -f workpay test_parse test_money test_cli test_quota
