#ifndef SC_MONEY_H
#define SC_MONEY_H

#include <stddef.h>

int format_money (char *output, size_t output_size, double amount,
                  const char *currency_code, int decimal_places);
double calculate_payment (int hours, int minutes, int seconds,
                          double hourly_rate);

#endif
