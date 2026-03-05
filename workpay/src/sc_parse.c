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

static int
is_leap_year (int year)
{
  return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

int
parse_date_ymd (const char *date_str, int *year, int *month, int *day)
{
  static const int DAYS_IN_MONTH[] = { 31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31 };
  int parsed_year;
  int parsed_month;
  int parsed_day;
  int month_days;
  size_t index;

  if (date_str == NULL || strlen (date_str) != 10)
    {
      return 0;
    }

  for (index = 0; index < 10; index++)
    {
      unsigned char ch = (unsigned char)date_str[index];
      if (index == 4 || index == 7)
        {
          if (ch != '-')
            {
              return 0;
            }
          continue;
        }

      if (!isdigit (ch))
        {
          return 0;
        }
    }

  parsed_year = (date_str[0] - '0') * 1000 + (date_str[1] - '0') * 100
                + (date_str[2] - '0') * 10 + (date_str[3] - '0');
  parsed_month = (date_str[5] - '0') * 10 + (date_str[6] - '0');
  parsed_day = (date_str[8] - '0') * 10 + (date_str[9] - '0');

  if (parsed_month < 1 || parsed_month > 12)
    {
      return 0;
    }

  month_days = DAYS_IN_MONTH[parsed_month - 1];
  if (parsed_month == 2 && is_leap_year (parsed_year))
    {
      month_days = 29;
    }

  if (parsed_day < 1 || parsed_day > month_days)
    {
      return 0;
    }

  if (year != NULL)
    {
      *year = parsed_year;
    }
  if (month != NULL)
    {
      *month = parsed_month;
    }
  if (day != NULL)
    {
      *day = parsed_day;
    }

  return 1;
}
