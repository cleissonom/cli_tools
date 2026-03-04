#ifndef SC_PARSE_H
#define SC_PARSE_H

#include <stddef.h>

int normalize_currency_code (const char *input, char *output, size_t output_size);
int parse_time (const char *time_str, int *hours, int *minutes, int *seconds);
int parse_hourly_rate (const char *hourly_rate_str, double *hourly_rate);

#endif
