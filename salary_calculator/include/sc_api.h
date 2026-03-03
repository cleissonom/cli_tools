#ifndef SC_API_H
#define SC_API_H

enum ApiRequestStatus
{
  API_REQUEST_OK = 0,
  API_REQUEST_QUOTA_REACHED,
  API_REQUEST_ERROR
};

enum PairValidationStatus
{
  PAIR_VALID = 0,
  PAIR_INVALID,
  PAIR_CHECK_ERROR
};

enum PairValidationStatus validate_currency_pair (const char *from_currency,
                                                  const char *to_currency,
                                                  enum ApiRequestStatus
                                                      *request_status);
enum ApiRequestStatus get_exchange_rate (const char *from_currency,
                                         const char *to_currency,
                                         double *exchange_rate);

#endif
