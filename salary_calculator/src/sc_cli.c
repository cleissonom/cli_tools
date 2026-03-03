#include "sc_cli.h"

#include "sc_config.h"
#include "sc_parse.h"

#include <stdio.h>
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

void
print_usage (FILE *stream, const char *program_name)
{
  fprintf (stream,
           "Usage: %s [--from CODE] [--to CODE] [--help] <hourly_rate> "
           "<hours:minutes:seconds>\n",
           program_name);
}

void
print_help (const char *program_name)
{
  print_usage (stdout, program_name);
  printf ("\n");
  printf ("Calculate earned amount based on worked time and hourly rate.\n");
  printf ("\n");
  printf ("Arguments:\n");
  printf ("  <hourly_rate>            Non-negative decimal number.\n");
  printf ("  <hours:minutes:seconds>  Time worked in HH:MM:SS format.\n");
  printf ("\n");
  printf ("Flags:\n");
  printf ("  -h, --help               Show this help message.\n");
  printf ("  --from CODE              Source currency (default: %s).\n",
          DEFAULT_FROM);
  printf ("  --to CODE                Target currency (default: %s).\n",
          DEFAULT_TO);
  printf ("\n");
  printf ("Currency pairs are validated against AwesomeAPI supported pairs.\n");
  printf ("Daily API limit: %d requests/day.\n", API_DAILY_REQUEST_LIMIT);
  printf ("Warnings are shown when 25, 10, 5, and 1 requests remain.\n");
  printf ("When the limit is reached, execution stops.\n");
  printf ("Reference list: %s/available\n", API_BASE_URL);
  printf ("\n");
  printf ("Examples:\n");
  printf ("  %s 25 08:30:45\n", program_name);
  printf ("  %s --from EUR --to USD 40 07:15:00\n", program_name);
  printf ("  %s --from BTC --to BRL 0.005 01:00:00\n", program_name);
}

enum CliParseStatus
parse_cli_arguments (int argc, char *argv[], char *from_currency,
                     size_t from_size, char *to_currency, size_t to_size,
                     const char **hourly_rate_arg, const char **time_arg,
                     const char *program_name)
{
  int index;
  int positional_count = 0;

  snprintf (from_currency, from_size, "%s", DEFAULT_FROM);
  snprintf (to_currency, to_size, "%s", DEFAULT_TO);

  *hourly_rate_arg = NULL;
  *time_arg = NULL;

  for (index = 1; index < argc; index++)
    {
      const char *argument = argv[index];

      if (strcmp (argument, "-h") == 0 || strcmp (argument, "--help") == 0)
        {
          return CLI_PARSE_SHOW_HELP;
        }

      if (strcmp (argument, "--from") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr,
                       "\033[1;31mMissing value for --from option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          if (!parse_currency_value ("--from", argv[++index], from_currency,
                                     from_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--from=", 7) == 0)
        {
          if (!parse_currency_value ("--from", argument + 7, from_currency,
                                     from_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strcmp (argument, "--to") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr,
                       "\033[1;31mMissing value for --to option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          if (!parse_currency_value ("--to", argv[++index], to_currency,
                                     to_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--to=", 5) == 0)
        {
          if (!parse_currency_value ("--to", argument + 5, to_currency,
                                     to_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--", 2) == 0)
        {
          fprintf (stderr, "\033[1;31mUnknown option: %s\033[0m\n", argument);
          return CLI_PARSE_ERROR;
        }

      if (positional_count == 0)
        {
          *hourly_rate_arg = argument;
          positional_count++;
          continue;
        }

      if (positional_count == 1)
        {
          *time_arg = argument;
          positional_count++;
          continue;
        }

      fprintf (stderr, "\033[1;31mToo many positional arguments.\033[0m\n");
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
