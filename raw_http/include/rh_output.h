#ifndef RH_OUTPUT_H
#define RH_OUTPUT_H

#include "rh_types.h"

#include <stdio.h>

void rh_write_output_stream (FILE *stream, const HttpResponse *response);
int rh_write_output_file (const char *output_filename,
                          const HttpResponse *response);

#endif
