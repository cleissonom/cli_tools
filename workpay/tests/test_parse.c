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

int
main (void)
{
  test_normalize_currency_code ();
  test_parse_time ();
  test_parse_hourly_rate ();

  printf ("All workpay parse tests passed.\n");
  return 0;
}
