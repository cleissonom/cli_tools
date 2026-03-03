#ifndef SC_API_H
#define SC_API_H

enum PairValidationStatus
{
  PAIR_VALID = 0,
  PAIR_INVALID,
  PAIR_CHECK_ERROR
};

extern int api_quota_reached_error;

enum PairValidationStatus validate_currency_pair (const char *from_currency,
                                                  const char *to_currency);
int get_exchange_rate (const char *from_currency, const char *to_currency,
                       double *exchange_rate);

#endif
