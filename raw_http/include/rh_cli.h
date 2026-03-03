#ifndef RH_CLI_H
#define RH_CLI_H

enum RhCliParseStatus
{
  RH_CLI_PARSE_OK = 0,
  RH_CLI_PARSE_ERROR
};

enum RhCliParseStatus rh_parse_cli_arguments (int argc, char *argv[],
                                              const char **input_filename,
                                              const char **output_filename,
                                              const char *program_name);

#endif
