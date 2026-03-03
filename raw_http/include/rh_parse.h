#ifndef RH_PARSE_H
#define RH_PARSE_H

#include "rh_types.h"

int rh_parse_request_file (const char *input_filename, HttpRequest *request);
void rh_parse_line (char *line, HttpRequest *request, int *is_body);
int rh_validate_required_fields (const HttpRequest *request);

#endif
