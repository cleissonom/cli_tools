#include "sc_api.h"
#include "sc_cli.h"
#include "sc_config.h"
#include "sc_money.h"
#include "sc_parse.h"

#include <stdio.h>

int
main (int argc, char *argv[])
{
  const char *hourly_rate_arg;
  const char *time_arg;
  char from_currency[MAX_CURRENCY_CODE];
  char to_currency[MAX_CURRENCY_CODE];
  enum CliParseStatus cli_status;
  enum PairValidationStatus pair_status;
  double hourly_rate;
  int hours, minutes, seconds;
  double total_earned_from_currency;
  double exchange_rate;
  double total_earned_to_currency;
  char formatted_total_from_currency[128];
  char formatted_total_to_currency[128];

  cli_status = parse_cli_arguments (argc, argv, from_currency,
                                    sizeof (from_currency), to_currency,
                                    sizeof (to_currency), &hourly_rate_arg,
                                    &time_arg, argv[0]);
  if (cli_status == CLI_PARSE_SHOW_HELP)
    {
      print_help (argv[0]);
      return 0;
    }

  if (cli_status == CLI_PARSE_ERROR)
    {
      return 1;
    }

  if (!parse_hourly_rate (hourly_rate_arg, &hourly_rate))
    {
      fprintf (stderr,
               "\033[1;31mInvalid hourly rate. Use a non-negative number.\033"
               "[0m\n");
      return 1;
    }

  if (!parse_time (time_arg, &hours, &minutes, &seconds))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid time format. Use HH:MM:SS with MM/SS between 00 "
          "and 59.\033[0m\n");
      return 1;
    }

  pair_status = validate_currency_pair (from_currency, to_currency);
  if (pair_status == PAIR_CHECK_ERROR)
    {
      if (api_quota_reached_error)
        {
          return 1;
        }
      fprintf (stderr,
               "\033[1;31mFailed to validate currency pair against "
               "AwesomeAPI.\033[0m\n");
      return 1;
    }

  if (pair_status == PAIR_INVALID)
    {
      fprintf (stderr, "\033[1;31mUnsupported currency pair: %s-%s\033[0m\n",
               from_currency, to_currency);
      return 1;
    }

  total_earned_from_currency
      = calculate_payment (hours, minutes, seconds, hourly_rate);

  if (!get_exchange_rate (from_currency, to_currency, &exchange_rate))
    {
      if (api_quota_reached_error)
        {
          return 1;
        }
      fprintf (stderr, "\033[1;31mFailed to retrieve exchange rate.\033[0m\n");
      return 1;
    }

  total_earned_to_currency = total_earned_from_currency * exchange_rate;

  if (!format_money (formatted_total_from_currency,
                     sizeof (formatted_total_from_currency),
                     total_earned_from_currency, from_currency, 2)
      || !format_money (formatted_total_to_currency,
                        sizeof (formatted_total_to_currency),
                        total_earned_to_currency, to_currency, 2))
    {
      fprintf (stderr, "\033[1;31mFailed to format currency values.\033[0m\n");
      return 1;
    }

  printf ("%sTotal hours worked:\033[0m %.2f hours\n", BLUE,
          (double)hours + (minutes / 60.0) + (seconds / 3600.0));
  printf ("%sHourly wage (%s):\033[0m %.6f %s\n", BLUE, from_currency,
          hourly_rate, from_currency);
  printf ("%sExchange rate (%s -> %s):\033[0m %.6f\n", YELLOW, from_currency,
          to_currency, exchange_rate);
  printf ("%sTotal earned in %s:\033[0m %s\n", YELLOW, from_currency,
          formatted_total_from_currency);
  printf ("%sTotal earned in %s:\033[0m %s\n", GREEN, to_currency,
          formatted_total_to_currency);

  return 0;
}
