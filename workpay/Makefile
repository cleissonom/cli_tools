CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LDLIBS = -lcurl -lm
SRC = src/main.c src/sc_parse.c src/sc_money.c src/sc_cli.c src/sc_quota.c src/sc_api.c src/sc_config.c src/sc_report_store.c src/sc_report_query.c src/sc_report_render.c
TEST_PARSE_SRC = tests/test_parse.c src/sc_parse.c
TEST_MONEY_SRC = tests/test_money.c src/sc_money.c
TEST_CLI_SRC = tests/test_cli.c src/sc_cli.c src/sc_parse.c
TEST_QUOTA_SRC = tests/test_quota.c src/sc_quota.c src/sc_config.c
TEST_REPORT_STORE_SRC = tests/test_report_store.c src/sc_report_store.c src/sc_parse.c
TEST_REPORT_QUERY_SRC = tests/test_report_query.c src/sc_report_query.c src/sc_report_store.c src/sc_parse.c
TEST_REPORT_EXPORT_SRC = tests/test_report_export.c src/sc_report_render.c src/sc_money.c

build:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o workpay $(SRC) $(LDLIBS)

test:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_parse $(TEST_PARSE_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_money $(TEST_MONEY_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_cli $(TEST_CLI_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_quota $(TEST_QUOTA_SRC)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_report_store $(TEST_REPORT_STORE_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_report_query $(TEST_REPORT_QUERY_SRC) -lm
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_report_export $(TEST_REPORT_EXPORT_SRC) -lm
	@./test_parse
	@./test_money
	@./test_cli
	@./test_quota
	@./test_report_store
	@./test_report_query
	@./test_report_export

clean:
	@rm -f workpay test_parse test_money test_cli test_quota test_report_store test_report_query test_report_export
