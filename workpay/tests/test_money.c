#include "sc_money.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void
test_calculate_payment (void)
{
  double total = calculate_payment (8, 30, 0, 25.0);
  assert (fabs (total - 212.5) < 1e-9);
}

static void
test_format_money_known_currency (void)
{
  char output[128];

  assert (format_money (output, sizeof (output), 1234.56, "USD", 2));
  assert (strcmp (output, "$1,234.56") == 0);

  assert (format_money (output, sizeof (output), 1234.56, "BRL", 2));
  assert (strcmp (output, "R$1.234,56") == 0);
}

static void
test_format_money_rounding_and_negative (void)
{
  char output[128];

  assert (format_money (output, sizeof (output), 1.995, "USD", 2));
  assert (strcmp (output, "$2.00") == 0);

  assert (format_money (output, sizeof (output), -9876.5, "USD", 2));
  assert (strcmp (output, "-$9,876.50") == 0);
}

static void
test_format_money_unknown_currency_and_validation (void)
{
  char output[128];

  assert (format_money (output, sizeof (output), 1234.5, "XYZ", 2));
  assert (strcmp (output, "1,234.50 XYZ") == 0);

  assert (format_money (output, sizeof (output), 42.0, "XYZ", 0));
  assert (strcmp (output, "42 XYZ") == 0);

  assert (!format_money (output, sizeof (output), 10.0, "USD", -1));
  assert (!format_money (output, sizeof (output), 10.0, "USD", 10));
  assert (!format_money (output, 4, 10.0, "USD", 2));
}

int
main (void)
{
  test_calculate_payment ();
  test_format_money_known_currency ();
  test_format_money_rounding_and_negative ();
  test_format_money_unknown_currency_and_validation ();

  printf ("All workpay money tests passed.\n");
  return 0;
}
