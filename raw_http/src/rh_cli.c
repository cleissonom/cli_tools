#include "rh_cli.h"

#include "rh_config.h"

#include <stdio.h>

enum RhCliParseStatus
rh_parse_cli_arguments (int argc, char *argv[], const char **input_filename,
                        const char **output_filename,
                        const char *program_name)
{
  if (argc != 3)
    {
      fprintf (
          stderr,
          RH_COLOR_ERROR "Usage: %s <input_file.txt> <output_file.txt>"
                         RH_COLOR_RESET "\n",
          program_name);
      return RH_CLI_PARSE_ERROR;
    }

  *input_filename = argv[1];
  *output_filename = argv[2];
  return RH_CLI_PARSE_OK;
}
