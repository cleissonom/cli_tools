#include "sc_report_store.h"

#include "sc_parse.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSV_FIELD_COUNT 12
#define CSV_FIELD_MAX 512

static void
set_error (char *error_output, size_t error_output_size, const char *format, ...)
{
  va_list args;

  if (error_output == NULL || error_output_size == 0)
    {
      return;
    }

  va_start (args, format);
  vsnprintf (error_output, error_output_size, format, args);
  va_end (args);
}

int
report_store_get_entries_file_path (char *output_path, size_t output_path_size)
{
  const char *custom_file_path = getenv ("WORKPAY_ENTRIES_FILE");
  const char *home_directory;
  int written_chars;

  if (output_path == NULL || output_path_size == 0)
    {
      return 0;
    }

  if (custom_file_path != NULL && custom_file_path[0] != '\0')
    {
      written_chars
          = snprintf (output_path, output_path_size, "%s", custom_file_path);
      return written_chars > 0 && (size_t)written_chars < output_path_size;
    }

  home_directory = getenv ("HOME");
  if (home_directory == NULL || home_directory[0] == '\0')
    {
      written_chars = snprintf (output_path, output_path_size, "./%s",
                                ENTRIES_FILE_NAME);
      return written_chars > 0 && (size_t)written_chars < output_path_size;
    }

  written_chars = snprintf (output_path, output_path_size, "%s/%s",
                            home_directory, ENTRIES_FILE_NAME);
  return written_chars > 0 && (size_t)written_chars < output_path_size;
}

static void
trim_line_endings (char *line)
{
  size_t length = strlen (line);

  while (length > 0
         && (line[length - 1] == '\n' || line[length - 1] == '\r'))
    {
      line[length - 1] = '\0';
      length--;
    }
}

static int
append_entry_to_list (struct ReportEntryList *entry_list,
                      const struct ReportEntry *entry)
{
  struct ReportEntry *new_items;
  size_t new_capacity;

  if (entry_list->count == entry_list->capacity)
    {
      new_capacity = entry_list->capacity == 0 ? 16 : entry_list->capacity * 2;
      new_items = realloc (entry_list->items,
                           new_capacity * sizeof (struct ReportEntry));
      if (new_items == NULL)
        {
          return 0;
        }

      entry_list->items = new_items;
      entry_list->capacity = new_capacity;
    }

  entry_list->items[entry_list->count++] = *entry;
  return 1;
}

void
report_entry_list_free (struct ReportEntryList *entry_list)
{
  if (entry_list == NULL)
    {
      return;
    }

  free (entry_list->items);
  entry_list->items = NULL;
  entry_list->count = 0;
  entry_list->capacity = 0;
}

static int
csv_field_needs_quotes (const char *field_value)
{
  size_t index;

  for (index = 0; field_value[index] != '\0'; index++)
    {
      if (field_value[index] == ',' || field_value[index] == '"'
          || field_value[index] == '\n' || field_value[index] == '\r')
        {
          return 1;
        }
    }

  return 0;
}

static int
write_csv_field (FILE *file, const char *field_value)
{
  size_t index;

  if (!csv_field_needs_quotes (field_value))
    {
      return fputs (field_value, file) >= 0;
    }

  if (fputc ('"', file) == EOF)
    {
      return 0;
    }

  for (index = 0; field_value[index] != '\0'; index++)
    {
      if (field_value[index] == '"')
        {
          if (fputc ('"', file) == EOF)
            {
              return 0;
            }
        }

      if (fputc (field_value[index], file) == EOF)
        {
          return 0;
        }
    }

  return fputc ('"', file) != EOF;
}

static int
write_csv_entry_row (FILE *file, const struct ReportEntry *entry)
{
  char entry_id_buffer[32];
  char hourly_rate_buffer[64];
  char hours_buffer[16];
  char minutes_buffer[16];
  char seconds_buffer[16];
  char total_from_buffer[64];
  char exchange_rate_buffer[64];
  char total_to_buffer[64];

  snprintf (entry_id_buffer, sizeof (entry_id_buffer), "%lld", entry->entry_id);
  snprintf (hourly_rate_buffer, sizeof (hourly_rate_buffer), "%.15g",
            entry->hourly_rate);
  snprintf (hours_buffer, sizeof (hours_buffer), "%d", entry->hours);
  snprintf (minutes_buffer, sizeof (minutes_buffer), "%d", entry->minutes);
  snprintf (seconds_buffer, sizeof (seconds_buffer), "%d", entry->seconds);
  snprintf (total_from_buffer, sizeof (total_from_buffer), "%.15g",
            entry->total_from_currency);
  snprintf (exchange_rate_buffer, sizeof (exchange_rate_buffer), "%.15g",
            entry->exchange_rate);
  snprintf (total_to_buffer, sizeof (total_to_buffer), "%.15g",
            entry->total_to_currency);

  if (!write_csv_field (file, entry_id_buffer) || fputc (',', file) == EOF
      || !write_csv_field (file, entry->created_at) || fputc (',', file) == EOF
      || !write_csv_field (file, entry->from_currency)
      || fputc (',', file) == EOF
      || !write_csv_field (file, entry->to_currency) || fputc (',', file) == EOF
      || !write_csv_field (file, hourly_rate_buffer)
      || fputc (',', file) == EOF || !write_csv_field (file, hours_buffer)
      || fputc (',', file) == EOF || !write_csv_field (file, minutes_buffer)
      || fputc (',', file) == EOF || !write_csv_field (file, seconds_buffer)
      || fputc (',', file) == EOF || !write_csv_field (file, total_from_buffer)
      || fputc (',', file) == EOF
      || !write_csv_field (file, exchange_rate_buffer)
      || fputc (',', file) == EOF || !write_csv_field (file, total_to_buffer)
      || fputc (',', file) == EOF || !write_csv_field (file, entry->tag)
      || fputc ('\n', file) == EOF)
    {
      return 0;
    }

  return 1;
}

enum ReportStoreStatus
report_store_append_entry (const struct ReportEntry *entry, char *error_output,
                           size_t error_output_size)
{
  char file_path[512];
  FILE *file;
  long file_size;

  if (entry == NULL)
    {
      set_error (error_output, error_output_size,
                 "Internal error: missing report entry.");
      return REPORT_STORE_WRITE_ERROR;
    }

  if (!report_store_get_entries_file_path (file_path, sizeof (file_path)))
    {
      set_error (error_output, error_output_size,
                 "Unable to resolve entries file path.");
      return REPORT_STORE_PATH_ERROR;
    }

  file = fopen (file_path, "a+");
  if (file == NULL)
    {
      set_error (error_output, error_output_size,
                 "Failed to open entries file for writing: %s", file_path);
      return REPORT_STORE_WRITE_ERROR;
    }

  if (fseek (file, 0, SEEK_END) != 0)
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Failed to inspect entries file: %s", file_path);
      return REPORT_STORE_WRITE_ERROR;
    }

  file_size = ftell (file);
  if (file_size < 0)
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Failed to inspect entries file: %s", file_path);
      return REPORT_STORE_WRITE_ERROR;
    }

  if (file_size == 0)
    {
      if (fprintf (file, "%s\n", REPORT_CSV_HEADER) < 0)
        {
          fclose (file);
          set_error (error_output, error_output_size,
                     "Failed to write CSV header to entries file: %s",
                     file_path);
          return REPORT_STORE_WRITE_ERROR;
        }
    }

  if (!write_csv_entry_row (file, entry))
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Failed to append entry to entries file: %s", file_path);
      return REPORT_STORE_WRITE_ERROR;
    }

  if (fclose (file) != 0)
    {
      set_error (error_output, error_output_size,
                 "Failed to finalize entries file write: %s", file_path);
      return REPORT_STORE_WRITE_ERROR;
    }

  return REPORT_STORE_OK;
}

static int
parse_long_long (const char *value, long long *output)
{
  char *endptr;
  long long parsed_value;

  errno = 0;
  parsed_value = strtoll (value, &endptr, 10);
  if (errno != 0 || endptr == value || *endptr != '\0')
    {
      return 0;
    }

  *output = parsed_value;
  return 1;
}

static int
parse_int (const char *value, int *output)
{
  char *endptr;
  long parsed_value;

  errno = 0;
  parsed_value = strtol (value, &endptr, 10);
  if (errno != 0 || endptr == value || *endptr != '\0'
      || parsed_value < -2147483648L || parsed_value > 2147483647L)
    {
      return 0;
    }

  *output = (int)parsed_value;
  return 1;
}

static int
parse_double (const char *value, double *output)
{
  char *endptr;
  double parsed_value;

  errno = 0;
  parsed_value = strtod (value, &endptr);
  if (errno != 0 || endptr == value || *endptr != '\0')
    {
      return 0;
    }

  *output = parsed_value;
  return 1;
}

static int
parse_csv_line (const char *line, char fields[][CSV_FIELD_MAX],
                size_t expected_fields)
{
  size_t source_index = 0;
  size_t field_index = 0;
  size_t field_length = 0;
  int in_quotes = 0;

  if (expected_fields == 0)
    {
      return 0;
    }

  fields[0][0] = '\0';

  while (1)
    {
      char ch = line[source_index];

      if (ch == '\0')
        {
          if (in_quotes)
            {
              return 0;
            }

          if (field_index != expected_fields - 1)
            {
              return 0;
            }

          fields[field_index][field_length] = '\0';
          return 1;
        }

      if (ch == '"')
        {
          if (in_quotes && line[source_index + 1] == '"')
            {
              if (field_length + 1 >= CSV_FIELD_MAX)
                {
                  return 0;
                }
              fields[field_index][field_length++] = '"';
              source_index += 2;
              continue;
            }

          in_quotes = !in_quotes;
          source_index++;
          continue;
        }

      if (ch == ',' && !in_quotes)
        {
          if (field_index + 1 >= expected_fields)
            {
              return 0;
            }

          fields[field_index][field_length] = '\0';
          field_index++;
          field_length = 0;
          fields[field_index][0] = '\0';
          source_index++;
          continue;
        }

      if (field_length + 1 >= CSV_FIELD_MAX)
        {
          return 0;
        }

      fields[field_index][field_length++] = ch;
      source_index++;
    }
}

static int
parse_entry_line (const char *line, struct ReportEntry *entry)
{
  char fields[CSV_FIELD_COUNT][CSV_FIELD_MAX];

  if (!parse_csv_line (line, fields, CSV_FIELD_COUNT))
    {
      return 0;
    }

  if (!parse_long_long (fields[0], &entry->entry_id))
    {
      return 0;
    }

  if (strlen (fields[1]) != 19 || fields[1][10] != 'T')
    {
      return 0;
    }

  {
    char created_date[REPORT_DATE_SIZE];
    int parsed_hours;
    int parsed_minutes;
    int parsed_seconds;

    snprintf (created_date, sizeof (created_date), "%.10s", fields[1]);
    if (!parse_date_ymd (created_date, NULL, NULL, NULL)
        || !parse_time (fields[1] + 11, &parsed_hours, &parsed_minutes,
                        &parsed_seconds))
      {
        return 0;
      }
  }

  if (!parse_double (fields[4], &entry->hourly_rate)
      || !parse_int (fields[5], &entry->hours)
      || !parse_int (fields[6], &entry->minutes)
      || !parse_int (fields[7], &entry->seconds)
      || !parse_double (fields[8], &entry->total_from_currency)
      || !parse_double (fields[9], &entry->exchange_rate)
      || !parse_double (fields[10], &entry->total_to_currency))
    {
      return 0;
    }

  if (entry->hours < 0 || entry->minutes < 0 || entry->minutes > 59
      || entry->seconds < 0 || entry->seconds > 59 || entry->hourly_rate < 0.0)
    {
      return 0;
    }

  if (!normalize_currency_code (fields[2], entry->from_currency,
                                sizeof (entry->from_currency))
      || !normalize_currency_code (fields[3], entry->to_currency,
                                   sizeof (entry->to_currency)))
    {
      return 0;
    }

  if (strlen (fields[11]) >= sizeof (entry->tag))
    {
      return 0;
    }

  snprintf (entry->created_at, sizeof (entry->created_at), "%s", fields[1]);
  snprintf (entry->tag, sizeof (entry->tag), "%s", fields[11]);
  return 1;
}

enum ReportStoreStatus
report_store_load_entries (struct ReportEntryList *entry_list,
                           char *error_output, size_t error_output_size)
{
  char file_path[512];
  FILE *file;
  char line_buffer[4096];
  size_t line_number = 1;

  if (entry_list == NULL)
    {
      set_error (error_output, error_output_size,
                 "Internal error: missing report list output.");
      return REPORT_STORE_READ_ERROR;
    }

  entry_list->items = NULL;
  entry_list->count = 0;
  entry_list->capacity = 0;

  if (!report_store_get_entries_file_path (file_path, sizeof (file_path)))
    {
      set_error (error_output, error_output_size,
                 "Unable to resolve entries file path.");
      return REPORT_STORE_PATH_ERROR;
    }

  file = fopen (file_path, "r");
  if (file == NULL)
    {
      if (errno == ENOENT)
        {
          return REPORT_STORE_OK;
        }

      set_error (error_output, error_output_size,
                 "Failed to open entries file for reading: %s", file_path);
      return REPORT_STORE_READ_ERROR;
    }

  if (fgets (line_buffer, sizeof (line_buffer), file) == NULL)
    {
      fclose (file);
      return REPORT_STORE_OK;
    }

  trim_line_endings (line_buffer);
  if (strcmp (line_buffer, REPORT_CSV_HEADER) != 0)
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Invalid entries CSV header in %s", file_path);
      return REPORT_STORE_PARSE_ERROR;
    }

  while (fgets (line_buffer, sizeof (line_buffer), file) != NULL)
    {
      struct ReportEntry entry;

      line_number++;
      trim_line_endings (line_buffer);
      if (line_buffer[0] == '\0')
        {
          continue;
        }

      if (!parse_entry_line (line_buffer, &entry))
        {
          fclose (file);
          report_entry_list_free (entry_list);
          set_error (error_output, error_output_size,
                     "Invalid CSV row at line %zu in %s", line_number,
                     file_path);
          return REPORT_STORE_PARSE_ERROR;
        }

      if (!append_entry_to_list (entry_list, &entry))
        {
          fclose (file);
          report_entry_list_free (entry_list);
          set_error (error_output, error_output_size,
                     "Out of memory while loading entries.");
          return REPORT_STORE_MEMORY_ERROR;
        }
    }

  if (ferror (file))
    {
      fclose (file);
      report_entry_list_free (entry_list);
      set_error (error_output, error_output_size,
                 "Failed to read entries file: %s", file_path);
      return REPORT_STORE_READ_ERROR;
    }

  fclose (file);
  return REPORT_STORE_OK;
}
