#include "sc_report_render.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
set_entry (struct ReportEntry *entry, long long id, const char *tag)
{
  memset (entry, 0, sizeof (*entry));
  entry->entry_id = id;
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
  snprintf (entry->tag, sizeof (entry->tag), "%s", tag);
}

static void
create_temp_file_path (char *output_path, size_t output_size)
{
  int fd;

  snprintf (output_path, output_size, "/tmp/workpay_export_test_XXXXXX");
  fd = mkstemp (output_path);
  assert (fd != -1);
  close (fd);
}

static void
read_file_content (const char *path, char *buffer, size_t buffer_size)
{
  FILE *file = fopen (path, "r");
  size_t read_bytes;

  assert (file != NULL);
  read_bytes = fread (buffer, 1, buffer_size - 1, file);
  buffer[read_bytes] = '\0';
  fclose (file);
}

static void
test_csv_export_writes_expected_header_and_rows (void)
{
  struct ReportEntry entries[2];
  char output_path[128];
  char content[4096];
  char error[256];

  create_temp_file_path (output_path, sizeof (output_path));
  set_entry (&entries[0], 1710000000, "alpha");
  set_entry (&entries[1], 1710000001, "beta");

  assert (report_export_entries_csv (output_path, entries, 2, error,
                                     sizeof (error)));

  read_file_content (output_path, content, sizeof (content));
  assert (strstr (content, REPORT_CSV_HEADER) == content);
  assert (strstr (content, "1710000000") != NULL);
  assert (strstr (content, "1710000001") != NULL);
  assert (strstr (content, "alpha") != NULL);
  assert (strstr (content, "beta") != NULL);

  assert (remove (output_path) == 0);
}

static void
test_json_export_writes_expected_fields (void)
{
  struct ReportEntry entries[1];
  char output_path[128];
  char content[4096];
  char error[256];

  create_temp_file_path (output_path, sizeof (output_path));
  set_entry (&entries[0], 1710000000, "alpha \"quoted\"");

  assert (report_export_entries_json (output_path, entries, 1, error,
                                      sizeof (error)));

  read_file_content (output_path, content, sizeof (content));
  assert (strstr (content, "\"entry_id\": 1710000000") != NULL);
  assert (strstr (content, "\"from_currency\": \"USD\"") != NULL);
  assert (strstr (content, "alpha \\\"quoted\\\"") != NULL);

  assert (remove (output_path) == 0);
}

static void
test_export_fails_for_unwritable_path (void)
{
  struct ReportEntry entry;
  char error[256];

  set_entry (&entry, 1710000000, "x");

  assert (!report_export_entries_csv ("/tmp/does-not-exist-dir/output.csv",
                                      &entry, 1, error, sizeof (error)));
  assert (strstr (error, "Failed to open export file") != NULL);
}

int
main (void)
{
  test_csv_export_writes_expected_header_and_rows ();
  test_json_export_writes_expected_fields ();
  test_export_fails_for_unwritable_path ();

  printf ("All workpay report export tests passed.\n");
  return 0;
}
