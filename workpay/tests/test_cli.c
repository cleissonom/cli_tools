#include "sc_cli.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
test_parse_defaults_with_positional_args (void)
{
  char from_currency[16];
  char to_currency[16];
  const char *hourly_rate_arg = NULL;
  const char *time_arg = NULL;
  char *argv[] = { "workpay", "25", "08:30:45" };

  assert (parse_cli_arguments (3, argv, from_currency, sizeof (from_currency),
                               to_currency, sizeof (to_currency),
                               &hourly_rate_arg, &time_arg, argv[0])
          == CLI_PARSE_OK);
  assert (strcmp (from_currency, "USD") == 0);
  assert (strcmp (to_currency, "BRL") == 0);
  assert (strcmp (hourly_rate_arg, "25") == 0);
  assert (strcmp (time_arg, "08:30:45") == 0);
}

static void
test_parse_with_currency_options (void)
{
  char from_currency[16];
  char to_currency[16];
  const char *hourly_rate_arg = NULL;
  const char *time_arg = NULL;
  char *argv[] = { "workpay", "--from=eur", "--to", "usd", "40",
                   "07:15:00" };

  assert (parse_cli_arguments (6, argv, from_currency, sizeof (from_currency),
                               to_currency, sizeof (to_currency),
                               &hourly_rate_arg, &time_arg, argv[0])
          == CLI_PARSE_OK);
  assert (strcmp (from_currency, "EUR") == 0);
  assert (strcmp (to_currency, "USD") == 0);
  assert (strcmp (hourly_rate_arg, "40") == 0);
  assert (strcmp (time_arg, "07:15:00") == 0);
}

static void
test_parse_help_and_invalid_inputs (void)
{
  char from_currency[16];
  char to_currency[16];
  const char *hourly_rate_arg = NULL;
  const char *time_arg = NULL;
  char *help_argv[] = { "workpay", "--help" };
  char *unknown_option_argv[] = { "workpay", "--bad", "20", "01:00:00" };
  char *missing_value_argv[] = { "workpay", "--from", "20", "01:00:00" };
  char *invalid_currency_argv[] = { "workpay", "--to", "u$d", "20",
                                    "01:00:00" };
  char *missing_positional_argv[] = { "workpay", "20" };
  char *extra_positional_argv[] = { "workpay", "20", "01:00:00", "extra" };

  assert (parse_cli_arguments (2, help_argv, from_currency, sizeof (from_currency),
                               to_currency, sizeof (to_currency),
                               &hourly_rate_arg, &time_arg, help_argv[0])
          == CLI_PARSE_SHOW_HELP);

  assert (parse_cli_arguments (4, unknown_option_argv, from_currency,
                               sizeof (from_currency), to_currency,
                               sizeof (to_currency), &hourly_rate_arg, &time_arg,
                               unknown_option_argv[0])
          == CLI_PARSE_ERROR);

  assert (parse_cli_arguments (4, missing_value_argv, from_currency,
                               sizeof (from_currency), to_currency,
                               sizeof (to_currency), &hourly_rate_arg, &time_arg,
                               missing_value_argv[0])
          == CLI_PARSE_ERROR);

  assert (parse_cli_arguments (5, invalid_currency_argv, from_currency,
                               sizeof (from_currency), to_currency,
                               sizeof (to_currency), &hourly_rate_arg, &time_arg,
                               invalid_currency_argv[0])
          == CLI_PARSE_ERROR);

  assert (parse_cli_arguments (2, missing_positional_argv, from_currency,
                               sizeof (from_currency), to_currency,
                               sizeof (to_currency), &hourly_rate_arg, &time_arg,
                               missing_positional_argv[0])
          == CLI_PARSE_ERROR);

  assert (parse_cli_arguments (4, extra_positional_argv, from_currency,
                               sizeof (from_currency), to_currency,
                               sizeof (to_currency), &hourly_rate_arg, &time_arg,
                               extra_positional_argv[0])
          == CLI_PARSE_ERROR);
}

int
main (void)
{
  test_parse_defaults_with_positional_args ();
  test_parse_with_currency_options ();
  test_parse_help_and_invalid_inputs ();

  printf ("All workpay CLI tests passed.\n");
  return 0;
}
