#include "rh_output.h"

#include "rh_config.h"

#include <stdio.h>

void
rh_write_output_stream (FILE *stream, const HttpResponse *response)
{
  fprintf (stream, "%s\n", response->headers);
  fprintf (stream, "%s", response->body);
}

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

  rh_write_output_stream (output_file, response);
  fclose (output_file);

  return 1;
}
