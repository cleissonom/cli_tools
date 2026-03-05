#include "sc_report_store.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
create_temp_entries_path (char *output_path, size_t output_size)
{
  int fd;

  snprintf (output_path, output_size, "/tmp/workpay_entries_test_XXXXXX");
  fd = mkstemp (output_path);
  assert (fd != -1);
  close (fd);
  assert (remove (output_path) == 0);
}

static void
populate_entry (struct ReportEntry *entry)
{
  memset (entry, 0, sizeof (*entry));
  entry->entry_id = 1710000000;
  snprintf (entry->created_at, sizeof (entry->created_at), "2026-03-04T10:11:12");
  snprintf (entry->from_currency, sizeof (entry->from_currency), "USD");
  snprintf (entry->to_currency, sizeof (entry->to_currency), "BRL");
  entry->hourly_rate = 25.5;
  entry->hours = 8;
  entry->minutes = 30;
  entry->seconds = 0;
  entry->total_from_currency = 216.75;
  entry->exchange_rate = 5.1234;
  entry->total_to_currency = 1110.49095;
  snprintf (entry->tag, sizeof (entry->tag), "daily-run");
}

static void
test_append_and_load_roundtrip (void)
{
  char path[128];
  char error[256];
  struct ReportEntry entry;
  struct ReportEntryList loaded;
  FILE *file;
  char header_line[512];

  create_temp_entries_path (path, sizeof (path));
  assert (setenv ("WORKPAY_ENTRIES_FILE", path, 1) == 0);

  populate_entry (&entry);
  assert (report_store_append_entry (&entry, error, sizeof (error))
          == REPORT_STORE_OK);

  file = fopen (path, "r");
  assert (file != NULL);
  assert (fgets (header_line, sizeof (header_line), file) != NULL);
  fclose (file);
  assert (strstr (header_line, REPORT_CSV_HEADER) == header_line);

  assert (report_store_load_entries (&loaded, error, sizeof (error))
          == REPORT_STORE_OK);
  assert (loaded.count == 1);
  assert (loaded.items[0].entry_id == entry.entry_id);
  assert (strcmp (loaded.items[0].created_at, entry.created_at) == 0);
  assert (strcmp (loaded.items[0].from_currency, "USD") == 0);
  assert (strcmp (loaded.items[0].to_currency, "BRL") == 0);
  assert (strcmp (loaded.items[0].tag, "daily-run") == 0);

  report_entry_list_free (&loaded);
  assert (unsetenv ("WORKPAY_ENTRIES_FILE") == 0);
  assert (remove (path) == 0);
}

static void
test_load_missing_file_returns_empty (void)
{
  char path[128];
  char error[256];
  struct ReportEntryList loaded;

  create_temp_entries_path (path, sizeof (path));
  assert (setenv ("WORKPAY_ENTRIES_FILE", path, 1) == 0);

  assert (report_store_load_entries (&loaded, error, sizeof (error))
          == REPORT_STORE_OK);
  assert (loaded.count == 0);
  report_entry_list_free (&loaded);

  assert (unsetenv ("WORKPAY_ENTRIES_FILE") == 0);
}

static void
test_invalid_header_returns_parse_error (void)
{
  char path[128];
  char error[256];
  struct ReportEntryList loaded;
  FILE *file;

  create_temp_entries_path (path, sizeof (path));
  assert (setenv ("WORKPAY_ENTRIES_FILE", path, 1) == 0);

  file = fopen (path, "w");
  assert (file != NULL);
  assert (fprintf (file, "bad,header\n") > 0);
  fclose (file);

  assert (report_store_load_entries (&loaded, error, sizeof (error))
          == REPORT_STORE_PARSE_ERROR);
  assert (strstr (error, "Invalid entries CSV header") != NULL);

  assert (unsetenv ("WORKPAY_ENTRIES_FILE") == 0);
  assert (remove (path) == 0);
}

int
main (void)
{
  test_append_and_load_roundtrip ();
  test_load_missing_file_returns_empty ();
  test_invalid_header_returns_parse_error ();

  printf ("All workpay report store tests passed.\n");
  return 0;
}
