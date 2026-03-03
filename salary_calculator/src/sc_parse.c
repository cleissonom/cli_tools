#include "sc_parse.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
normalize_currency_code (const char *input, char *output, size_t output_size)
{
  size_t i;
  size_t input_length;

  if (input == NULL || output == NULL || output_size < 2)
    {
      return 0;
    }

  input_length = strlen (input);
  if (input_length < 3 || input_length >= output_size)
    {
      return 0;
    }

  for (i = 0; i < input_length; i++)
    {
      unsigned char ch = (unsigned char)input[i];
      if (!isalnum (ch))
        {
          return 0;
        }
      output[i] = (char)toupper (ch);
    }

  output[input_length] = '\0';
  return 1;
}

int
parse_time (const char *time_str, int *hours, int *minutes, int *seconds)
{
  char extra_char;
  int parsed_items
      = sscanf (time_str, "%d:%d:%d%c", hours, minutes, seconds, &extra_char);

  if (parsed_items != 3)
    {
      return 0;
    }

  if (*hours < 0 || *minutes < 0 || *minutes > 59 || *seconds < 0
      || *seconds > 59)
    {
      return 0;
    }

  return 1;
}

int
parse_hourly_rate (const char *hourly_rate_str, double *hourly_rate)
{
  char *endptr;
  double parsed_rate;

  errno = 0;
  parsed_rate = strtod (hourly_rate_str, &endptr);

  if (errno != 0 || endptr == hourly_rate_str || *endptr != '\0'
      || parsed_rate < 0.0)
    {
      return 0;
    }

  *hourly_rate = parsed_rate;
  return 1;
}
