#include "sc_cli.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
test_parse_calculation_defaults (void)
{
  struct CliArguments arguments;
  char *argv[] = { "workpay", "25", "08:30:45" };

  assert (parse_cli_arguments (3, argv, &arguments, argv[0]) == CLI_PARSE_OK);
  assert (arguments.command_type == CLI_COMMAND_CALCULATION);
  assert (strcmp (arguments.from_currency, "USD") == 0);
  assert (strcmp (arguments.to_currency, "BRL") == 0);
  assert (arguments.has_from_option == 0);
  assert (arguments.has_to_option == 0);
  assert (strcmp (arguments.hourly_rate_arg, "25") == 0);
  assert (strcmp (arguments.time_arg, "08:30:45") == 0);
  assert (arguments.save_entry == 0);
  assert (arguments.tag == NULL);
  assert (arguments.save_date == NULL);
  assert (arguments.has_exchange_rate == 0);
  assert (arguments.exchange_rate == NULL);
  assert (arguments.has_extra_from_amount == 0);
  assert (arguments.extra_from_amount == NULL);
}

static void
test_parse_calculation_with_save_and_tag (void)
{
  struct CliArguments arguments;
  char *argv[] = { "workpay", "--from=eur", "--to", "usd", "--save",
                   "--tag", "invoice-42", "--save-date", "2026-03-01",
                   "--exchange-rate", "5.1", "40", "07:15:00" };

  assert (parse_cli_arguments (13, argv, &arguments, argv[0]) == CLI_PARSE_OK);
  assert (arguments.command_type == CLI_COMMAND_CALCULATION);
  assert (strcmp (arguments.from_currency, "EUR") == 0);
  assert (strcmp (arguments.to_currency, "USD") == 0);
  assert (arguments.has_from_option == 1);
  assert (arguments.has_to_option == 1);
  assert (arguments.save_entry == 1);
  assert (strcmp (arguments.tag, "invoice-42") == 0);
  assert (strcmp (arguments.save_date, "2026-03-01") == 0);
  assert (arguments.has_exchange_rate == 1);
  assert (strcmp (arguments.exchange_rate, "5.1") == 0);
  assert (arguments.has_extra_from_amount == 0);
  assert (arguments.extra_from_amount == NULL);
  assert (strcmp (arguments.hourly_rate_arg, "40") == 0);
  assert (strcmp (arguments.time_arg, "07:15:00") == 0);
}

static void
test_parse_report_summary (void)
{
  struct CliArguments arguments;
  char *argv[] = { "workpay",      "report",   "summary",  "--from-date",
                   "2026-01-01",   "--to-date", "2026-01-31", "--from",
                   "usd",          "--to",      "brl" };

  assert (parse_cli_arguments (11, argv, &arguments, argv[0]) == CLI_PARSE_OK);
  assert (arguments.command_type == CLI_COMMAND_REPORT_SUMMARY);
  assert (strcmp (arguments.from_date, "2026-01-01") == 0);
  assert (strcmp (arguments.to_date, "2026-01-31") == 0);
  assert (strcmp (arguments.from_currency, "USD") == 0);
  assert (strcmp (arguments.to_currency, "BRL") == 0);
  assert (arguments.has_from_option == 1);
  assert (arguments.has_to_option == 1);
  assert (arguments.has_limit == 0);
}

static void
test_parse_calculation_with_save_datetime (void)
{
  struct CliArguments arguments;
  char *argv[]
      = { "workpay", "--save", "--save-date=2026-03-01T12:34:56",
          "--exchange-rate=5.25", "20", "01:00:00" };

  assert (parse_cli_arguments (6, argv, &arguments, argv[0]) == CLI_PARSE_OK);
  assert (arguments.command_type == CLI_COMMAND_CALCULATION);
  assert (arguments.save_entry == 1);
  assert (strcmp (arguments.save_date, "2026-03-01T12:34:56") == 0);
  assert (arguments.has_exchange_rate == 1);
  assert (strcmp (arguments.exchange_rate, "5.25") == 0);
  assert (arguments.has_extra_from_amount == 0);
  assert (arguments.extra_from_amount == NULL);
}

static void
test_parse_calculation_with_extra_from (void)
{
  struct CliArguments arguments;
  char *argv[] = { "workpay", "--from", "usd", "--to", "brl",
                   "--extra-from=120.5", "23", "154:22:12" };

  assert (parse_cli_arguments (8, argv, &arguments, argv[0]) == CLI_PARSE_OK);
  assert (arguments.command_type == CLI_COMMAND_CALCULATION);
  assert (arguments.has_extra_from_amount == 1);
  assert (strcmp (arguments.extra_from_amount, "120.5") == 0);
  assert (strcmp (arguments.hourly_rate_arg, "23") == 0);
  assert (strcmp (arguments.time_arg, "154:22:12") == 0);
}

static void
test_parse_report_list_and_export (void)
{
  struct CliArguments list_arguments;
  struct CliArguments export_arguments;
  char *list_argv[] = { "workpay", "report", "list", "--from", "usd",
                        "--limit", "10" };
  char *export_argv[] = { "workpay", "report", "export", "--format", "json",
                          "--output", "/tmp/report.json", "--to", "eur" };

  assert (parse_cli_arguments (7, list_argv, &list_arguments, list_argv[0])
          == CLI_PARSE_OK);
  assert (list_arguments.command_type == CLI_COMMAND_REPORT_LIST);
  assert (list_arguments.has_limit == 1);
  assert (list_arguments.limit == 10);
  assert (list_arguments.has_from_option == 1);
  assert (strcmp (list_arguments.from_currency, "USD") == 0);

  assert (parse_cli_arguments (9, export_argv, &export_arguments,
                               export_argv[0])
          == CLI_PARSE_OK);
  assert (export_arguments.command_type == CLI_COMMAND_REPORT_EXPORT);
  assert (export_arguments.export_format == REPORT_EXPORT_FORMAT_JSON);
  assert (strcmp (export_arguments.output_path, "/tmp/report.json") == 0);
  assert (export_arguments.has_to_option == 1);
  assert (strcmp (export_arguments.to_currency, "EUR") == 0);
}

static void
test_parse_help_and_invalid_inputs (void)
{
  struct CliArguments arguments;
  char *help_argv[] = { "workpay", "--help" };
  char *missing_subcommand_argv[] = { "workpay", "report" };
  char *unknown_subcommand_argv[] = { "workpay", "report", "bad" };
  char *tag_without_save_argv[] = { "workpay", "--tag", "x", "20", "01:00:00" };
  char *save_date_without_save_argv[] = { "workpay", "--save-date", "2026-03-01",
                                          "20", "01:00:00" };
  char *save_date_without_exchange_rate_argv[]
      = { "workpay", "--save", "--save-date", "2026-03-01", "20", "01:00:00" };
  char *exchange_rate_without_save_date_argv[]
      = { "workpay", "--save", "--exchange-rate", "5.1", "20", "01:00:00" };
  char *invalid_exchange_rate_argv[] = { "workpay", "--save", "--save-date",
                                         "2026-03-01", "--exchange-rate", "-1",
                                         "20", "01:00:00" };
  char *missing_extra_from_value_argv[] = { "workpay", "--extra-from" };
  char *invalid_extra_from_argv[] = { "workpay", "--extra-from", "-10", "20",
                                      "01:00:00" };
  char *invalid_save_date_argv[] = { "workpay", "--save", "--save-date",
                                     "2026/03/01", "20", "01:00:00" };
  char *invalid_date_argv[] = { "workpay", "report", "summary", "--from-date",
                                "2026-02-30" };
  char *invalid_limit_argv[] = { "workpay", "report", "list", "--limit", "0" };
  char *missing_export_output_argv[] = { "workpay", "report", "export",
                                         "--format", "csv" };
  char *missing_export_format_argv[] = { "workpay", "report", "export",
                                         "--output", "/tmp/out.csv" };
  char *unknown_option_argv[] = { "workpay", "--bad", "20", "01:00:00" };

  assert (parse_cli_arguments (2, help_argv, &arguments, help_argv[0])
          == CLI_PARSE_SHOW_HELP);
  assert (parse_cli_arguments (2, missing_subcommand_argv, &arguments,
                               missing_subcommand_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (3, unknown_subcommand_argv, &arguments,
                               unknown_subcommand_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, tag_without_save_argv, &arguments,
                               tag_without_save_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, save_date_without_save_argv, &arguments,
                               save_date_without_save_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (6, save_date_without_exchange_rate_argv,
                               &arguments,
                               save_date_without_exchange_rate_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (6, exchange_rate_without_save_date_argv,
                               &arguments,
                               exchange_rate_without_save_date_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (8, invalid_exchange_rate_argv, &arguments,
                               invalid_exchange_rate_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (2, missing_extra_from_value_argv, &arguments,
                               missing_extra_from_value_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, invalid_extra_from_argv, &arguments,
                               invalid_extra_from_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (6, invalid_save_date_argv, &arguments,
                               invalid_save_date_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, invalid_date_argv, &arguments,
                               invalid_date_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, invalid_limit_argv, &arguments,
                               invalid_limit_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, missing_export_output_argv, &arguments,
                               missing_export_output_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (5, missing_export_format_argv, &arguments,
                               missing_export_format_argv[0])
          == CLI_PARSE_ERROR);
  assert (parse_cli_arguments (4, unknown_option_argv, &arguments,
                               unknown_option_argv[0])
          == CLI_PARSE_ERROR);
}

int
main (void)
{
  test_parse_calculation_defaults ();
  test_parse_calculation_with_save_and_tag ();
  test_parse_calculation_with_save_datetime ();
  test_parse_calculation_with_extra_from ();
  test_parse_report_summary ();
  test_parse_report_list_and_export ();
  test_parse_help_and_invalid_inputs ();

  printf ("All workpay CLI tests passed.\n");
  return 0;
}
