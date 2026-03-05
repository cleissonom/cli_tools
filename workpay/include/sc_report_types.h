#ifndef SC_REPORT_TYPES_H
#define SC_REPORT_TYPES_H

#include "sc_config.h"

#include <stddef.h>

#define REPORT_CREATED_AT_SIZE 20
#define REPORT_DATE_SIZE 11
#define REPORT_TAG_SIZE 128
#define REPORT_CSV_HEADER                                                     \
  "entry_id,created_at,from_currency,to_currency,hourly_rate,hours,minutes," \
  "seconds,total_from_currency,exchange_rate,total_to_currency,tag"

struct ReportEntry
{
  long long entry_id;
  char created_at[REPORT_CREATED_AT_SIZE];
  char from_currency[MAX_CURRENCY_CODE];
  char to_currency[MAX_CURRENCY_CODE];
  double hourly_rate;
  int hours;
  int minutes;
  int seconds;
  double total_from_currency;
  double exchange_rate;
  double total_to_currency;
  char tag[REPORT_TAG_SIZE];
};

struct ReportEntryList
{
  struct ReportEntry *items;
  size_t count;
  size_t capacity;
};

struct ReportFilter
{
  const char *from_date;
  const char *to_date;
  int has_from_currency;
  char from_currency[MAX_CURRENCY_CODE];
  int has_to_currency;
  char to_currency[MAX_CURRENCY_CODE];
  int has_limit;
  int limit;
};

struct ReportSummaryGroup
{
  char from_currency[MAX_CURRENCY_CODE];
  char to_currency[MAX_CURRENCY_CODE];
  size_t count;
  double total_from_currency;
  double total_to_currency;
  double exchange_rate_sum;
};

struct ReportSummary
{
  size_t total_entries;
  struct ReportSummaryGroup *groups;
  size_t group_count;
  size_t group_capacity;
};

#endif
