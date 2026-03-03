#include "rh_output.h"

#include "rh_config.h"

#include <stdio.h>

int
rh_write_output_file (const char *output_filename, const HttpResponse *response)
{
  FILE *output_file = fopen (output_filename, "w");

  if (output_file == NULL)
    {
      fprintf (stderr,
               RH_COLOR_ERROR "Error: Could not open output file %s"
                              RH_COLOR_RESET "\n",
               output_filename);
      return 0;
    }

  fprintf (output_file, "%s\n", response->headers);
  fprintf (output_file, "%s", response->body);
  fclose (output_file);

  return 1;
}
