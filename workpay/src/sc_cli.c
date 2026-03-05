#include "sc_cli.h"

#include "sc_config.h"
#include "sc_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
parse_currency_value (const char *option_name, const char *value,
                      char *output_currency, size_t output_size)
{
  if (!normalize_currency_code (value, output_currency, output_size))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid value for %s. Use only letters and digits "
          "(example: USD, BRL, BRLT, BTC).\033[0m\n",
          option_name);
      return 0;
    }

  return 1;
}

static int
parse_date_value (const char *option_name, const char *value)
{
  if (!parse_date_ymd (value, NULL, NULL, NULL))
    {
      fprintf (stderr,
               "\033[1;31mInvalid value for %s. Use YYYY-MM-DD.\033[0m\n",
               option_name);
      return 0;
    }

  return 1;
}

static int
parse_save_date_value (const char *value)
{
  char date_part[11];
  int hours;
  int minutes;
  int seconds;

  if (strlen (value) == 10)
    {
      if (!parse_date_ymd (value, NULL, NULL, NULL))
        {
          fprintf (stderr,
                   "\033[1;31mInvalid value for --save-date. Use YYYY-MM-DD "
                   "or YYYY-MM-DDTHH:MM:SS.\033[0m\n");
          return 0;
        }

      return 1;
    }

  if (strlen (value) != 19 || value[10] != 'T')
    {
      fprintf (stderr,
               "\033[1;31mInvalid value for --save-date. Use YYYY-MM-DD or "
               "YYYY-MM-DDTHH:MM:SS.\033[0m\n");
      return 0;
    }

  snprintf (date_part, sizeof (date_part), "%.10s", value);
  if (!parse_date_ymd (date_part, NULL, NULL, NULL)
      || !parse_time (value + 11, &hours, &minutes, &seconds))
    {
      fprintf (stderr,
               "\033[1;31mInvalid value for --save-date. Use YYYY-MM-DD or "
               "YYYY-MM-DDTHH:MM:SS.\033[0m\n");
      return 0;
    }

  return 1;
}

static int
parse_limit_value (const char *value, int *limit)
{
  char *endptr;
  long parsed_limit;

  parsed_limit = strtol (value, &endptr, 10);
  if (endptr == value || *endptr != '\0' || parsed_limit <= 0
      || parsed_limit > 2147483647L)
    {
      fprintf (stderr,
               "\033[1;31mInvalid value for --limit. Use a positive "
               "integer.\033[0m\n");
      return 0;
    }

  *limit = (int)parsed_limit;
  return 1;
}

static int
parse_exchange_rate_value (const char *value)
{
  char *endptr;
  double parsed_value;

  parsed_value = strtod (value, &endptr);
  if (endptr == value || *endptr != '\0' || parsed_value <= 0.0)
    {
      fprintf (stderr,
               "\033[1;31mInvalid value for --exchange-rate. Use a positive "
               "number.\033[0m\n");
      return 0;
    }

  return 1;
}

static int
parse_extra_from_value (const char *value)
{
  char *endptr;
  double parsed_value;

  parsed_value = strtod (value, &endptr);
  if (endptr == value || *endptr != '\0' || parsed_value < 0.0)
    {
      fprintf (stderr,
               "\033[1;31mInvalid value for --extra-from. Use a non-negative "
               "number.\033[0m\n");
      return 0;
    }

  return 1;
}

static int
parse_format_value (const char *value, enum ReportExportFormat *format)
{
  if (strcmp (value, "csv") == 0)
    {
      *format = REPORT_EXPORT_FORMAT_CSV;
      return 1;
    }

  if (strcmp (value, "json") == 0)
    {
      *format = REPORT_EXPORT_FORMAT_JSON;
      return 1;
    }

  fprintf (stderr,
           "\033[1;31mInvalid value for --format. Use csv or json.\033[0m\n");
  return 0;
}

void
print_usage (FILE *stream, const char *program_name)
{
  fprintf (
      stream,
      "Usage:\n"
      "  %s [--from CODE] [--to CODE] [--save] [--tag TAG] "
      "[--save-date DATE] [--exchange-rate RATE] [--extra-from AMOUNT] "
      "<hourly_rate> <hours:minutes:seconds>\n"
      "  %s report summary [--from-date YYYY-MM-DD] [--to-date YYYY-MM-DD] "
      "[--from CODE] [--to CODE]\n"
      "  %s report list [--from-date YYYY-MM-DD] [--to-date YYYY-MM-DD] "
      "[--from CODE] [--to CODE] [--limit N]\n"
      "  %s report export --format csv|json --output <PATH> "
      "[--from-date YYYY-MM-DD] [--to-date YYYY-MM-DD] [--from CODE] "
      "[--to CODE]\n",
      program_name, program_name, program_name, program_name);
}

void
print_help (const char *program_name)
{
  print_usage (stdout, program_name);
  printf ("\n");
  printf ("Calculate earned amount or generate reports from saved entries.\n");
  printf ("\n");
  printf ("Calculation arguments:\n");
  printf ("  <hourly_rate>            Non-negative decimal number.\n");
  printf ("  <hours:minutes:seconds>  Time worked in HH:MM:SS format.\n");
  printf ("\n");
  printf ("Calculation flags:\n");
  printf ("  --from CODE              Source currency (default: %s).\n",
          DEFAULT_FROM);
  printf ("  --to CODE                Target currency (default: %s).\n",
          DEFAULT_TO);
  printf ("  --save                   Persist successful calculation to CSV storage.\n");
  printf ("  --tag TAG                Optional label for saved entries (requires --save).\n");
  printf ("  --save-date DATE         Override save timestamp (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS; requires --save).\n");
  printf ("  --exchange-rate RATE     Exchange rate to use with --save-date (required with --save-date).\n");
  printf ("  --extra-from AMOUNT      Extra earnings in source currency to add to the calculated total.\n");
  printf ("\n");
  printf ("Report flags:\n");
  printf ("  --from-date YYYY-MM-DD   Include entries created on/after date.\n");
  printf ("  --to-date YYYY-MM-DD     Include entries created on/before date.\n");
  printf ("  --limit N                Maximum entries for report list.\n");
  printf ("  --format csv|json        Export format for report export.\n");
  printf ("  --output <PATH>          Export destination for report export.\n");
  printf ("\n");
  printf ("General flags:\n");
  printf ("  -h, --help               Show this help message.\n");
  printf ("\n");
  printf ("Currency pairs are validated against AwesomeAPI supported pairs.\n");
  printf ("Daily API limit: %d requests/day.\n", API_DAILY_REQUEST_LIMIT);
  printf ("Warnings are shown when 25, 10, 5, and 1 requests remain.\n");
  printf ("When the limit is reached, execution stops.\n");
  printf ("Reference list: %s/available\n", API_BASE_URL);
  printf ("\n");
  printf ("Examples:\n");
  printf ("  %s 25 08:30:45\n", program_name);
  printf ("  %s --from EUR --to USD --save --tag invoice-42 40 07:15:00\n",
          program_name);
  printf ("  %s --save --save-date 2026-03-01 --exchange-rate 5.10 25 08:30:45\n",
          program_name);
  printf ("  %s --from USD --to BRL --extra-from 120 23 154:22:12\n",
          program_name);
  printf ("  %s report summary --from-date 2026-01-01 --to-date 2026-01-31\n",
          program_name);
  printf ("  %s report list --from USD --to BRL --limit 10\n", program_name);
  printf ("  %s report export --format json --output /tmp/workpay.json --from USD\n",
          program_name);
}

static void
init_arguments (struct CliArguments *arguments)
{
  arguments->command_type = CLI_COMMAND_CALCULATION;
  snprintf (arguments->from_currency, sizeof (arguments->from_currency), "%s",
            DEFAULT_FROM);
  snprintf (arguments->to_currency, sizeof (arguments->to_currency), "%s",
            DEFAULT_TO);
  arguments->has_from_option = 0;
  arguments->has_to_option = 0;
  arguments->hourly_rate_arg = NULL;
  arguments->time_arg = NULL;
  arguments->save_entry = 0;
  arguments->tag = NULL;
  arguments->save_date = NULL;
  arguments->has_exchange_rate = 0;
  arguments->exchange_rate = NULL;
  arguments->has_extra_from_amount = 0;
  arguments->extra_from_amount = NULL;
  arguments->from_date = NULL;
  arguments->to_date = NULL;
  arguments->has_limit = 0;
  arguments->limit = 0;
  arguments->export_format = REPORT_EXPORT_FORMAT_NONE;
  arguments->output_path = NULL;
}

static int
parse_currency_option (const char *argument, int *index, int argc, char *argv[],
                       struct CliArguments *arguments)
{
  if (strcmp (argument, "--from") == 0)
    {
      if (*index + 1 >= argc)
        {
          fprintf (stderr, "\033[1;31mMissing value for --from option.\033[0m\n");
          return 0;
        }

      return parse_currency_value ("--from", argv[++(*index)],
                                   arguments->from_currency,
                                   sizeof (arguments->from_currency))
             ? (arguments->has_from_option = 1)
             : 0;
    }

  if (strncmp (argument, "--from=", 7) == 0)
    {
      return parse_currency_value ("--from", argument + 7,
                                   arguments->from_currency,
                                   sizeof (arguments->from_currency))
             ? (arguments->has_from_option = 1)
             : 0;
    }

  if (strcmp (argument, "--to") == 0)
    {
      if (*index + 1 >= argc)
        {
          fprintf (stderr, "\033[1;31mMissing value for --to option.\033[0m\n");
          return 0;
        }

      return parse_currency_value ("--to", argv[++(*index)], arguments->to_currency,
                                   sizeof (arguments->to_currency))
             ? (arguments->has_to_option = 1)
             : 0;
    }

  if (strncmp (argument, "--to=", 5) == 0)
    {
      return parse_currency_value ("--to", argument + 5, arguments->to_currency,
                                   sizeof (arguments->to_currency))
             ? (arguments->has_to_option = 1)
             : 0;
    }

  return -1;
}

static enum CliParseStatus
parse_calculation_arguments (int argc, char *argv[],
                             struct CliArguments *arguments,
                             const char *program_name)
{
  int index;
  int positional_count = 0;

  for (index = 1; index < argc; index++)
    {
      const char *argument = argv[index];
      int currency_result;

      if (strcmp (argument, "-h") == 0 || strcmp (argument, "--help") == 0)
        {
          return CLI_PARSE_SHOW_HELP;
        }

      currency_result
          = parse_currency_option (argument, &index, argc, argv, arguments);
      if (currency_result == 0)
        {
          return CLI_PARSE_ERROR;
        }
      if (currency_result == 1)
        {
          continue;
        }

      if (strcmp (argument, "--save") == 0)
        {
          arguments->save_entry = 1;
          continue;
        }

      if (strcmp (argument, "--tag") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr, "\033[1;31mMissing value for --tag option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          arguments->tag = argv[++index];
          continue;
        }

      if (strncmp (argument, "--tag=", 6) == 0)
        {
          arguments->tag = argument + 6;
          continue;
        }

      if (strcmp (argument, "--save-date") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr,
                       "\033[1;31mMissing value for --save-date option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          arguments->save_date = argv[++index];
          if (!parse_save_date_value (arguments->save_date))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--save-date=", 12) == 0)
        {
          arguments->save_date = argument + 12;
          if (!parse_save_date_value (arguments->save_date))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strcmp (argument, "--exchange-rate") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr, "\033[1;31mMissing value for --exchange-rate option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          arguments->exchange_rate = argv[++index];
          if (!parse_exchange_rate_value (arguments->exchange_rate))
            {
              return CLI_PARSE_ERROR;
            }
          arguments->has_exchange_rate = 1;
          continue;
        }

      if (strncmp (argument, "--exchange-rate=", 16) == 0)
        {
          arguments->exchange_rate = argument + 16;
          if (!parse_exchange_rate_value (arguments->exchange_rate))
            {
              return CLI_PARSE_ERROR;
            }
          arguments->has_exchange_rate = 1;
          continue;
        }

      if (strcmp (argument, "--extra-from") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr,
                       "\033[1;31mMissing value for --extra-from option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          arguments->extra_from_amount = argv[++index];
          if (!parse_extra_from_value (arguments->extra_from_amount))
            {
              return CLI_PARSE_ERROR;
            }
          arguments->has_extra_from_amount = 1;
          continue;
        }

      if (strncmp (argument, "--extra-from=", 13) == 0)
        {
          arguments->extra_from_amount = argument + 13;
          if (!parse_extra_from_value (arguments->extra_from_amount))
            {
              return CLI_PARSE_ERROR;
            }
          arguments->has_extra_from_amount = 1;
          continue;
        }

      if (strncmp (argument, "--", 2) == 0)
        {
          fprintf (stderr, "\033[1;31mUnknown option: %s\033[0m\n", argument);
          return CLI_PARSE_ERROR;
        }

      if (positional_count == 0)
        {
          arguments->hourly_rate_arg = argument;
          positional_count++;
          continue;
        }

      if (positional_count == 1)
        {
          arguments->time_arg = argument;
          positional_count++;
          continue;
        }

      fprintf (stderr, "\033[1;31mToo many positional arguments.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (arguments->tag != NULL && !arguments->save_entry)
    {
      fprintf (stderr,
               "\033[1;31m--tag can only be used together with --save.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (arguments->save_date != NULL && !arguments->save_entry)
    {
      fprintf (stderr,
               "\033[1;31m--save-date can only be used together with "
               "--save.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (arguments->save_date != NULL && !arguments->has_exchange_rate)
    {
      fprintf (stderr,
               "\033[1;31m--exchange-rate is required when --save-date is "
               "provided.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (arguments->has_exchange_rate && arguments->save_date == NULL)
    {
      fprintf (stderr,
               "\033[1;31m--exchange-rate can only be used together with "
               "--save-date.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (positional_count != 2)
    {
      fprintf (stderr, "\033[1;31m");
      print_usage (stderr, program_name);
      fprintf (stderr, "\033[0m");
      return CLI_PARSE_ERROR;
    }

  return CLI_PARSE_OK;
}

static int
parse_report_common_option (const char *argument, int *index, int argc,
                            char *argv[], struct CliArguments *arguments)
{
  int currency_result
      = parse_currency_option (argument, index, argc, argv, arguments);
  if (currency_result != -1)
    {
      return currency_result;
    }

  if (strcmp (argument, "--from-date") == 0)
    {
      if (*index + 1 >= argc)
        {
          fprintf (stderr,
                   "\033[1;31mMissing value for --from-date option.\033[0m\n");
          return 0;
        }

      arguments->from_date = argv[++(*index)];
      return parse_date_value ("--from-date", arguments->from_date);
    }

  if (strncmp (argument, "--from-date=", 12) == 0)
    {
      arguments->from_date = argument + 12;
      return parse_date_value ("--from-date", arguments->from_date);
    }

  if (strcmp (argument, "--to-date") == 0)
    {
      if (*index + 1 >= argc)
        {
          fprintf (stderr,
                   "\033[1;31mMissing value for --to-date option.\033[0m\n");
          return 0;
        }

      arguments->to_date = argv[++(*index)];
      return parse_date_value ("--to-date", arguments->to_date);
    }

  if (strncmp (argument, "--to-date=", 10) == 0)
    {
      arguments->to_date = argument + 10;
      return parse_date_value ("--to-date", arguments->to_date);
    }

  return -1;
}

static enum CliParseStatus
parse_report_arguments (int argc, char *argv[], struct CliArguments *arguments)
{
  int index;

  if (argc < 3)
    {
      fprintf (stderr,
               "\033[1;31mMissing report subcommand. Use summary, list, or "
               "export.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (strcmp (argv[2], "summary") == 0)
    {
      arguments->command_type = CLI_COMMAND_REPORT_SUMMARY;
    }
  else if (strcmp (argv[2], "list") == 0)
    {
      arguments->command_type = CLI_COMMAND_REPORT_LIST;
    }
  else if (strcmp (argv[2], "export") == 0)
    {
      arguments->command_type = CLI_COMMAND_REPORT_EXPORT;
    }
  else
    {
      fprintf (stderr,
               "\033[1;31mUnknown report subcommand: %s. Use summary, list, "
               "or export.\033[0m\n",
               argv[2]);
      return CLI_PARSE_ERROR;
    }

  for (index = 3; index < argc; index++)
    {
      const char *argument = argv[index];
      int common_result;

      if (strcmp (argument, "-h") == 0 || strcmp (argument, "--help") == 0)
        {
          return CLI_PARSE_SHOW_HELP;
        }

      common_result
          = parse_report_common_option (argument, &index, argc, argv, arguments);
      if (common_result == 0)
        {
          return CLI_PARSE_ERROR;
        }
      if (common_result == 1)
        {
          continue;
        }

      if (arguments->command_type == CLI_COMMAND_REPORT_LIST)
        {
          if (strcmp (argument, "--limit") == 0)
            {
              if (index + 1 >= argc)
                {
                  fprintf (
                      stderr,
                      "\033[1;31mMissing value for --limit option.\033[0m\n");
                  return CLI_PARSE_ERROR;
                }
              arguments->has_limit = 1;
              if (!parse_limit_value (argv[++index], &arguments->limit))
                {
                  return CLI_PARSE_ERROR;
                }
              continue;
            }

          if (strncmp (argument, "--limit=", 8) == 0)
            {
              arguments->has_limit = 1;
              if (!parse_limit_value (argument + 8, &arguments->limit))
                {
                  return CLI_PARSE_ERROR;
                }
              continue;
            }
        }

      if (arguments->command_type == CLI_COMMAND_REPORT_EXPORT)
        {
          if (strcmp (argument, "--format") == 0)
            {
              if (index + 1 >= argc)
                {
                  fprintf (
                      stderr,
                      "\033[1;31mMissing value for --format option.\033[0m\n");
                  return CLI_PARSE_ERROR;
                }
              if (!parse_format_value (argv[++index], &arguments->export_format))
                {
                  return CLI_PARSE_ERROR;
                }
              continue;
            }

          if (strncmp (argument, "--format=", 9) == 0)
            {
              if (!parse_format_value (argument + 9, &arguments->export_format))
                {
                  return CLI_PARSE_ERROR;
                }
              continue;
            }

          if (strcmp (argument, "--output") == 0)
            {
              if (index + 1 >= argc)
                {
                  fprintf (
                      stderr,
                      "\033[1;31mMissing value for --output option.\033[0m\n");
                  return CLI_PARSE_ERROR;
                }
              arguments->output_path = argv[++index];
              continue;
            }

          if (strncmp (argument, "--output=", 9) == 0)
            {
              arguments->output_path = argument + 9;
              continue;
            }
        }

      fprintf (stderr, "\033[1;31mUnknown option: %s\033[0m\n", argument);
      return CLI_PARSE_ERROR;
    }

  if (arguments->command_type == CLI_COMMAND_REPORT_EXPORT)
    {
      if (arguments->export_format == REPORT_EXPORT_FORMAT_NONE)
        {
          fprintf (stderr,
                   "\033[1;31mMissing required option --format for report "
                   "export.\033[0m\n");
          return CLI_PARSE_ERROR;
        }

      if (arguments->output_path == NULL || arguments->output_path[0] == '\0')
        {
          fprintf (stderr,
                   "\033[1;31mMissing required option --output for report "
                   "export.\033[0m\n");
          return CLI_PARSE_ERROR;
        }
    }

  return CLI_PARSE_OK;
}

enum CliParseStatus
parse_cli_arguments (int argc, char *argv[], struct CliArguments *arguments,
                     const char *program_name)
{
  if (arguments == NULL)
    {
      fprintf (stderr, "\033[1;31mInternal CLI parser error.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  init_arguments (arguments);

  if (argc < 2)
    {
      fprintf (stderr, "\033[1;31m");
      print_usage (stderr, program_name);
      fprintf (stderr, "\033[0m");
      return CLI_PARSE_ERROR;
    }

  if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0)
    {
      return CLI_PARSE_SHOW_HELP;
    }

  if (strcmp (argv[1], "report") == 0)
    {
      return parse_report_arguments (argc, argv, arguments);
    }

  return parse_calculation_arguments (argc, argv, arguments, program_name);
}
