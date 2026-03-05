#include "sc_report_query.h"
#include "sc_report_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
set_entry (struct ReportEntry *entry, long long id, const char *created_at,
           const char *from_currency, const char *to_currency, double total_from,
           double rate, double total_to)
{
  memset (entry, 0, sizeof (*entry));
  entry->entry_id = id;
  snprintf (entry->created_at, sizeof (entry->created_at), "%s", created_at);
  snprintf (entry->from_currency, sizeof (entry->from_currency), "%s",
            from_currency);
  snprintf (entry->to_currency, sizeof (entry->to_currency), "%s", to_currency);
  entry->hourly_rate = 10.0;
  entry->hours = 1;
  entry->minutes = 0;
  entry->seconds = 0;
  entry->total_from_currency = total_from;
  entry->exchange_rate = rate;
  entry->total_to_currency = total_to;
  entry->tag[0] = '\0';
}

static void
test_filter_by_date_currency_and_limit (void)
{
  struct ReportEntry entries[3];
  struct ReportFilter filter;
  struct ReportEntryList filtered;
  char error[256];

  set_entry (&entries[0], 1, "2026-01-01T10:00:00", "USD", "BRL", 100.0, 5.0,
             500.0);
  set_entry (&entries[1], 2, "2026-01-05T10:00:00", "USD", "BRL", 50.0, 5.2,
             260.0);
  set_entry (&entries[2], 3, "2026-02-01T10:00:00", "EUR", "USD", 70.0, 1.07,
             74.9);

  report_filter_init (&filter);
  filter.from_date = "2026-01-01";
  filter.to_date = "2026-01-31";
  filter.has_from_currency = 1;
  snprintf (filter.from_currency, sizeof (filter.from_currency), "USD");
  filter.has_to_currency = 1;
  snprintf (filter.to_currency, sizeof (filter.to_currency), "BRL");
  filter.has_limit = 1;
  filter.limit = 1;

  assert (report_filter_entries (entries, 3, &filter, &filtered, error,
                                 sizeof (error)));
  assert (filtered.count == 1);
  assert (filtered.items[0].entry_id == 1);

  report_entry_list_free (&filtered);
}

static void
test_invalid_date_range_is_rejected (void)
{
  struct ReportFilter filter;
  char error[256];

  report_filter_init (&filter);
  filter.from_date = "2026-03-01";
  filter.to_date = "2026-02-01";

  assert (!report_validate_filter (&filter, error, sizeof (error)));
  assert (strstr (error, "Invalid date range") != NULL);
}

static void
test_summary_groups_by_currency_pair (void)
{
  struct ReportEntry entries[3];
  struct ReportSummary summary;
  char error[256];

  set_entry (&entries[0], 1, "2026-01-01T10:00:00", "USD", "BRL", 100.0, 5.0,
             500.0);
  set_entry (&entries[1], 2, "2026-01-05T10:00:00", "USD", "BRL", 50.0, 5.2,
             260.0);
  set_entry (&entries[2], 3, "2026-02-01T10:00:00", "EUR", "USD", 70.0, 1.07,
             74.9);

  assert (report_build_summary (entries, 3, &summary, error, sizeof (error)));
  assert (summary.total_entries == 3);
  assert (summary.group_count == 2);

  assert (strcmp (summary.groups[0].from_currency, "USD") == 0);
  assert (strcmp (summary.groups[0].to_currency, "BRL") == 0);
  assert (summary.groups[0].count == 2);
  assert (summary.groups[0].total_from_currency > 149.99
          && summary.groups[0].total_from_currency < 150.01);
  assert (summary.groups[0].total_to_currency > 759.99
          && summary.groups[0].total_to_currency < 760.01);

  assert (strcmp (summary.groups[1].from_currency, "EUR") == 0);
  assert (strcmp (summary.groups[1].to_currency, "USD") == 0);
  assert (summary.groups[1].count == 1);

  report_summary_free (&summary);
}

int
main (void)
{
  test_filter_by_date_currency_and_limit ();
  test_invalid_date_range_is_rejected ();
  test_summary_groups_by_currency_pair ();

  printf ("All workpay report query tests passed.\n");
  return 0;
}
