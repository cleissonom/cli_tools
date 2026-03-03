#ifndef SC_CLI_H
#define SC_CLI_H

#include <stddef.h>
#include <stdio.h>

enum CliParseStatus
{
  CLI_PARSE_OK = 0,
  CLI_PARSE_SHOW_HELP,
  CLI_PARSE_ERROR
};

void print_usage (FILE *stream, const char *program_name);
void print_help (const char *program_name);
enum CliParseStatus parse_cli_arguments (int argc, char *argv[],
                                         char *from_currency,
                                         size_t from_size,
                                         char *to_currency, size_t to_size,
                                         const char **hourly_rate_arg,
                                         const char **time_arg,
                                         const char *program_name);

#endif
