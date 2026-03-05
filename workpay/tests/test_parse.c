#include "sc_parse.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void
test_normalize_currency_code (void)
{
  char output[16];

  assert (normalize_currency_code ("usd", output, sizeof (output)));
  assert (strcmp (output, "USD") == 0);

  assert (normalize_currency_code ("brlt", output, sizeof (output)));
  assert (strcmp (output, "BRLT") == 0);

  assert (!normalize_currency_code ("us", output, sizeof (output)));
  assert (!normalize_currency_code ("usd$", output, sizeof (output)));
  assert (!normalize_currency_code ("usd", output, 3));
}

static void
test_parse_time (void)
{
  int hours = 0;
  int minutes = 0;
  int seconds = 0;

  assert (parse_time ("08:30:45", &hours, &minutes, &seconds));
  assert (hours == 8);
  assert (minutes == 30);
  assert (seconds == 45);

  assert (!parse_time ("08:61:00", &hours, &minutes, &seconds));
  assert (!parse_time ("-01:30:00", &hours, &minutes, &seconds));
  assert (!parse_time ("08:30", &hours, &minutes, &seconds));
  assert (!parse_time ("08:30:45Z", &hours, &minutes, &seconds));
}

static void
test_parse_hourly_rate (void)
{
  double value = 0.0;

  assert (parse_hourly_rate ("25", &value));
  assert (fabs (value - 25.0) < 1e-9);

  assert (parse_hourly_rate ("0.005", &value));
  assert (fabs (value - 0.005) < 1e-12);

  assert (!parse_hourly_rate ("-1", &value));
  assert (!parse_hourly_rate ("abc", &value));
  assert (!parse_hourly_rate ("12.4x", &value));
}

static void
test_parse_date_ymd (void)
{
  int year = 0;
  int month = 0;
  int day = 0;

  assert (parse_date_ymd ("2026-03-04", &year, &month, &day));
  assert (year == 2026);
  assert (month == 3);
  assert (day == 4);

  assert (parse_date_ymd ("2024-02-29", &year, &month, &day));
  assert (!parse_date_ymd ("2025-02-29", &year, &month, &day));
  assert (!parse_date_ymd ("2026-13-01", &year, &month, &day));
  assert (!parse_date_ymd ("2026-00-01", &year, &month, &day));
  assert (!parse_date_ymd ("2026-01-32", &year, &month, &day));
  assert (!parse_date_ymd ("20260304", &year, &month, &day));
  assert (!parse_date_ymd ("2026-3-04", &year, &month, &day));
}

int
main (void)
{
  test_normalize_currency_code ();
  test_parse_time ();
  test_parse_hourly_rate ();
  test_parse_date_ymd ();

  printf ("All workpay parse tests passed.\n");
  return 0;
}
