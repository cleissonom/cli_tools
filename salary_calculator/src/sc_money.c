#include "sc_money.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

struct CurrencyFormat
{
  const char *code;
  const char *symbol;
  char decimal_separator;
  char thousands_separator;
  int space_after_symbol;
};

static const struct CurrencyFormat CURRENCY_FORMATS[] = {
  { "USD", "$", '.', ',', 0 },      { "BRL", "R$", ',', '.', 1 },
  { "BRLT", "R$", ',', '.', 1 },    { "BRLPTAX", "R$", ',', '.', 1 },
  { "EUR", "€", ',', '.', 1 },      { "GBP", "£", '.', ',', 0 },
  { "JPY", "¥", '.', ',', 0 },      { "CAD", "C$", '.', ',', 0 },
  { "AUD", "A$", '.', ',', 0 },     { "NZD", "NZ$", '.', ',', 0 },
  { "CHF", "CHF", '.', ',', 1 },    { "CNY", "¥", '.', ',', 0 },
  { "HKD", "HK$", '.', ',', 0 },    { "SGD", "S$", '.', ',', 0 },
  { "MXN", "MX$", '.', ',', 0 },    { "ARS", "AR$", ',', '.', 0 },
  { "CLP", "CLP$", ',', '.', 0 },   { "COP", "COP$", ',', '.', 0 },
  { "PEN", "S/", ',', '.', 0 },     { "PYG", "₲", ',', '.', 0 },
  { "UYU", "$U", ',', '.', 0 },     { "BOB", "Bs", ',', '.', 1 },
  { "RUB", "₽", ',', '.', 0 },      { "TRY", "₺", ',', '.', 0 },
  { "PLN", "zł", ',', '.', 0 },     { "SEK", "kr", ',', '.', 1 },
  { "NOK", "kr", ',', '.', 1 },     { "DKK", "kr", ',', '.', 1 },
  { "CZK", "Kč", ',', '.', 1 },     { "HUF", "Ft", ',', '.', 1 },
  { "INR", "₹", '.', ',', 0 },      { "KRW", "₩", '.', ',', 0 },
  { "TWD", "NT$", '.', ',', 0 },    { "THB", "฿", '.', ',', 0 },
  { "ILS", "₪", '.', ',', 0 },      { "AED", "AED", '.', ',', 1 },
  { "SAR", "SAR", '.', ',', 1 },    { "QAR", "QAR", '.', ',', 1 },
  { "KWD", "KWD", '.', ',', 1 },    { "JOD", "JOD", '.', ',', 1 },
  { "BTC", "BTC", '.', ',', 1 },    { "ETH", "ETH", '.', ',', 1 },
  { "LTC", "LTC", '.', ',', 1 },    { "XRP", "XRP", '.', ',', 1 },
  { "DOGE", "DOGE", '.', ',', 1 },  { "SOL", "SOL", '.', ',', 1 },
  { "BNB", "BNB", '.', ',', 1 },    { "BRETT", "BRETT", '.', ',', 1 },
  { "XAU", "XAU", '.', ',', 1 },    { "XAG", "XAG", '.', ',', 1 },
  { "XAGG", "XAGG", '.', ',', 1 },  { "XBR", "XBR", '.', ',', 1 }
};

static const struct CurrencyFormat *
find_currency_format (const char *currency_code)
{
  size_t index;

  for (index = 0; index < sizeof (CURRENCY_FORMATS) / sizeof (CURRENCY_FORMATS[0]);
       index++)
    {
      if (strcmp (CURRENCY_FORMATS[index].code, currency_code) == 0)
        {
          return &CURRENCY_FORMATS[index];
        }
    }

  return NULL;
}

static int
format_grouped_integer (unsigned long long value, char thousands_separator,
                        char *output, size_t output_size)
{
  char digits[32];
  size_t digits_length;
  size_t output_length;
  size_t source_index;
  size_t destination_index;
  int group_count = 0;

  snprintf (digits, sizeof (digits), "%llu", value);
  digits_length = strlen (digits);
  output_length = digits_length + ((digits_length - 1) / 3);

  if (output_length + 1 > output_size)
    {
      return 0;
    }

  output[output_length] = '\0';

  source_index = digits_length;
  destination_index = output_length;

  while (source_index > 0)
    {
      output[--destination_index] = digits[--source_index];
      group_count++;

      if (group_count == 3 && source_index > 0)
        {
          output[--destination_index] = thousands_separator;
          group_count = 0;
        }
    }

  return 1;
}

int
format_money (char *output, size_t output_size, double amount,
              const char *currency_code, int decimal_places)
{
  const struct CurrencyFormat *format;
  char integer_part_buffer[64];
  char decimal_part_buffer[32];
  unsigned long long scaled_value;
  unsigned long long integer_part;
  unsigned long long decimal_part;
  unsigned long long decimal_base = 1;
  double absolute_amount;
  int is_negative;
  int index;
  int decimal_result;
  int written_chars;

  if (decimal_places < 0 || decimal_places > 9)
    {
      return 0;
    }

  format = find_currency_format (currency_code);
  is_negative = amount < 0.0;
  absolute_amount = fabs (amount);

  for (index = 0; index < decimal_places; index++)
    {
      decimal_base *= 10ULL;
    }

  scaled_value = (unsigned long long)llround (absolute_amount * decimal_base);
  integer_part = scaled_value / decimal_base;
  decimal_part = scaled_value % decimal_base;

  if (!format_grouped_integer (
          integer_part,
          format != NULL ? format->thousands_separator : ',',
          integer_part_buffer, sizeof (integer_part_buffer)))
    {
      return 0;
    }

  if (decimal_places > 0)
    {
      decimal_result = snprintf (decimal_part_buffer, sizeof (decimal_part_buffer),
                                 "%0*llu", decimal_places, decimal_part);
      if (decimal_result <= 0
          || (size_t)decimal_result >= sizeof (decimal_part_buffer))
        {
          return 0;
        }
    }
  else
    {
      decimal_part_buffer[0] = '\0';
    }

  if (format != NULL)
    {
      if (decimal_places > 0)
        {
          written_chars = snprintf (
              output, output_size, "%s%s%s%s%c%s",
              is_negative ? "-" : "", format->symbol,
              format->space_after_symbol ? " " : "", integer_part_buffer,
              format->decimal_separator, decimal_part_buffer);
        }
      else
        {
          written_chars = snprintf (output, output_size, "%s%s%s%s",
                                    is_negative ? "-" : "", format->symbol,
                                    format->space_after_symbol ? " " : "",
                                    integer_part_buffer);
        }
    }
  else
    {
      if (decimal_places > 0)
        {
          written_chars
              = snprintf (output, output_size, "%s%s%c%s %s",
                          is_negative ? "-" : "", integer_part_buffer, '.',
                          decimal_part_buffer, currency_code);
        }
      else
        {
          written_chars = snprintf (output, output_size, "%s%s %s",
                                    is_negative ? "-" : "", integer_part_buffer,
                                    currency_code);
        }
    }

  if (written_chars <= 0 || (size_t)written_chars >= output_size)
    {
      return 0;
    }

  return 1;
}

double
calculate_payment (int hours, int minutes, int seconds, double hourly_rate)
{
  double total_hours = hours + (minutes / 60.0) + (seconds / 3600.0);
  return total_hours * hourly_rate;
}
