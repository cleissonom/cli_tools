#ifndef SC_CLI_H
#define SC_CLI_H

#include "sc_config.h"

#include <stddef.h>
#include <stdio.h>

enum CliParseStatus
{
  CLI_PARSE_OK = 0,
  CLI_PARSE_SHOW_HELP,
  CLI_PARSE_ERROR
};

enum CliCommandType
{
  CLI_COMMAND_CALCULATION = 0,
  CLI_COMMAND_REPORT_SUMMARY,
  CLI_COMMAND_REPORT_LIST,
  CLI_COMMAND_REPORT_EXPORT
};

enum ReportExportFormat
{
  REPORT_EXPORT_FORMAT_NONE = 0,
  REPORT_EXPORT_FORMAT_CSV,
  REPORT_EXPORT_FORMAT_JSON
};

struct CliArguments
{
  enum CliCommandType command_type;
  char from_currency[MAX_CURRENCY_CODE];
  char to_currency[MAX_CURRENCY_CODE];
  int has_from_option;
  int has_to_option;
  const char *hourly_rate_arg;
  const char *time_arg;
  int save_entry;
  const char *tag;
  const char *save_date;
  int has_exchange_rate;
  const char *exchange_rate;
  int has_extra_from_amount;
  const char *extra_from_amount;
  const char *from_date;
  const char *to_date;
  int has_limit;
  int limit;
  enum ReportExportFormat export_format;
  const char *output_path;
};

void print_usage (FILE *stream, const char *program_name);
void print_help (const char *program_name);
enum CliParseStatus parse_cli_arguments (int argc, char *argv[],
                                         struct CliArguments *arguments,
                                         const char *program_name);

#endif
