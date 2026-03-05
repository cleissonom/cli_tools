#include "sc_report_render.h"

#include "sc_money.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

void
report_render_summary (FILE *stream, const struct ReportSummary *summary)
{
  size_t index;

  fprintf (stream, "Entries found: %zu\n", summary->total_entries);
  fprintf (stream,
           "%-8s %-8s %-8s %-20s %-20s %-14s\n"
           "%-8s %-8s %-8s %-20s %-20s %-14s\n",
           "FROM", "TO", "COUNT", "TOTAL_FROM", "TOTAL_TO", "AVG_RATE",
           "--------", "--------", "--------", "--------------------",
           "--------------------", "--------------");

  for (index = 0; index < summary->group_count; index++)
    {
      const struct ReportSummaryGroup *group = &summary->groups[index];
      double average_rate
          = group->count == 0 ? 0.0 : group->exchange_rate_sum / group->count;
      char total_from_formatted[128];
      char total_to_formatted[128];
      int from_format_ok
          = format_money (total_from_formatted, sizeof (total_from_formatted),
                          group->total_from_currency, group->from_currency, 2);
      int to_format_ok
          = format_money (total_to_formatted, sizeof (total_to_formatted),
                          group->total_to_currency, group->to_currency, 2);

      if (!from_format_ok)
        {
          snprintf (total_from_formatted, sizeof (total_from_formatted),
                    "%.2f %s", group->total_from_currency, group->from_currency);
        }
      if (!to_format_ok)
        {
          snprintf (total_to_formatted, sizeof (total_to_formatted), "%.2f %s",
                    group->total_to_currency, group->to_currency);
        }

      fprintf (stream,
               "%-8s %-8s %-8zu %-20s %-20s %-14.6f\n", group->from_currency,
               group->to_currency, group->count, total_from_formatted,
               total_to_formatted, average_rate);
    }
}

void
report_render_list (FILE *stream, const struct ReportEntry *entries,
                    size_t entry_count)
{
  size_t index;

  fprintf (
      stream,
      "%-12s %-19s %-6s %-6s %-8s %-16s %-20s %-14s %-20s %s\n"
      "%-12s %-19s %-6s %-6s %-8s %-16s %-20s %-14s %-20s %s\n",
      "ENTRY_ID", "CREATED_AT", "FROM", "TO", "TIME", "HOURLY", "TOTAL_FROM",
      "EXCHANGE", "TOTAL_TO", "TAG", "------------", "-------------------",
      "------", "------", "--------", "----------------", "--------------------",
      "--------------", "--------------------", "---");

  for (index = 0; index < entry_count; index++)
    {
      char time_buffer[16];
      char total_from_formatted[128];
      char total_to_formatted[128];
      int from_format_ok;
      int to_format_ok;

      snprintf (time_buffer, sizeof (time_buffer), "%02d:%02d:%02d",
                entries[index].hours, entries[index].minutes,
                entries[index].seconds);
      from_format_ok
          = format_money (total_from_formatted, sizeof (total_from_formatted),
                          entries[index].total_from_currency,
                          entries[index].from_currency, 2);
      to_format_ok
          = format_money (total_to_formatted, sizeof (total_to_formatted),
                          entries[index].total_to_currency,
                          entries[index].to_currency, 2);

      if (!from_format_ok)
        {
          snprintf (total_from_formatted, sizeof (total_from_formatted),
                    "%.2f %s", entries[index].total_from_currency,
                    entries[index].from_currency);
        }
      if (!to_format_ok)
        {
          snprintf (total_to_formatted, sizeof (total_to_formatted), "%.2f %s",
                    entries[index].total_to_currency, entries[index].to_currency);
        }

      fprintf (stream,
               "%-12lld %-19s %-6s %-6s %-8s %-16.6f %-20s %-14.6f "
               "%-20s %s\n",
               entries[index].entry_id, entries[index].created_at,
               entries[index].from_currency, entries[index].to_currency,
               time_buffer, entries[index].hourly_rate,
               total_from_formatted, entries[index].exchange_rate,
               total_to_formatted, entries[index].tag);
    }
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
      if (field_value[index] == '"' && fputc ('"', file) == EOF)
        {
          return 0;
        }

      if (fputc (field_value[index], file) == EOF)
        {
          return 0;
        }
    }

  return fputc ('"', file) != EOF;
}

static int
write_entry_as_csv_row (FILE *file, const struct ReportEntry *entry)
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

int
report_export_entries_csv (const char *output_path,
                           const struct ReportEntry *entries, size_t entry_count,
                           char *error_output, size_t error_output_size)
{
  FILE *file;
  size_t index;

  file = fopen (output_path, "w");
  if (file == NULL)
    {
      set_error (error_output, error_output_size,
                 "Failed to open export file for writing: %s", output_path);
      return 0;
    }

  if (fprintf (file, "%s\n", REPORT_CSV_HEADER) < 0)
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Failed to write CSV header to export file: %s", output_path);
      return 0;
    }

  for (index = 0; index < entry_count; index++)
    {
      if (!write_entry_as_csv_row (file, &entries[index]))
        {
          fclose (file);
          set_error (error_output, error_output_size,
                     "Failed to write CSV export row to %s", output_path);
          return 0;
        }
    }

  if (fclose (file) != 0)
    {
      set_error (error_output, error_output_size,
                 "Failed to finalize CSV export file: %s", output_path);
      return 0;
    }

  return 1;
}

static int
write_json_escaped_string (FILE *file, const char *value)
{
  size_t index;

  if (fputc ('"', file) == EOF)
    {
      return 0;
    }

  for (index = 0; value[index] != '\0'; index++)
    {
      char ch = value[index];

      if (ch == '"' || ch == '\\')
        {
          if (fputc ('\\', file) == EOF)
            {
              return 0;
            }
        }
      else if (ch == '\n')
        {
          if (fputs ("\\n", file) < 0)
            {
              return 0;
            }
          continue;
        }
      else if (ch == '\r')
        {
          if (fputs ("\\r", file) < 0)
            {
              return 0;
            }
          continue;
        }
      else if (ch == '\t')
        {
          if (fputs ("\\t", file) < 0)
            {
              return 0;
            }
          continue;
        }

      if (fputc (ch, file) == EOF)
        {
          return 0;
        }
    }

  return fputc ('"', file) != EOF;
}

int
report_export_entries_json (const char *output_path,
                            const struct ReportEntry *entries,
                            size_t entry_count, char *error_output,
                            size_t error_output_size)
{
  FILE *file;
  size_t index;

  file = fopen (output_path, "w");
  if (file == NULL)
    {
      set_error (error_output, error_output_size,
                 "Failed to open export file for writing: %s", output_path);
      return 0;
    }

  if (fputs ("[\n", file) < 0)
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Failed to initialize JSON export file: %s", output_path);
      return 0;
    }

  for (index = 0; index < entry_count; index++)
    {
      if (fputs ("  {\n", file) < 0
          || fputs ("    \"entry_id\": ", file) < 0
          || fprintf (file, "%lld", entries[index].entry_id) < 0
          || fputs (",\n    \"created_at\": ", file) < 0
          || !write_json_escaped_string (file, entries[index].created_at)
          || fputs (",\n    \"from_currency\": ", file) < 0
          || !write_json_escaped_string (file, entries[index].from_currency)
          || fputs (",\n    \"to_currency\": ", file) < 0
          || !write_json_escaped_string (file, entries[index].to_currency)
          || fputs (",\n    \"hourly_rate\": ", file) < 0
          || fprintf (file, "%.15g", entries[index].hourly_rate) < 0
          || fputs (",\n    \"hours\": ", file) < 0
          || fprintf (file, "%d", entries[index].hours) < 0
          || fputs (",\n    \"minutes\": ", file) < 0
          || fprintf (file, "%d", entries[index].minutes) < 0
          || fputs (",\n    \"seconds\": ", file) < 0
          || fprintf (file, "%d", entries[index].seconds) < 0
          || fputs (",\n    \"total_from_currency\": ", file) < 0
          || fprintf (file, "%.15g", entries[index].total_from_currency) < 0
          || fputs (",\n    \"exchange_rate\": ", file) < 0
          || fprintf (file, "%.15g", entries[index].exchange_rate) < 0
          || fputs (",\n    \"total_to_currency\": ", file) < 0
          || fprintf (file, "%.15g", entries[index].total_to_currency) < 0
          || fputs (",\n    \"tag\": ", file) < 0
          || !write_json_escaped_string (file, entries[index].tag)
          || fputs ("\n  }", file) < 0)
        {
          fclose (file);
          set_error (error_output, error_output_size,
                     "Failed to write JSON export row to %s", output_path);
          return 0;
        }

      if (index + 1 < entry_count)
        {
          if (fputs (",\n", file) < 0)
            {
              fclose (file);
              set_error (error_output, error_output_size,
                         "Failed to write JSON export row delimiter to %s",
                         output_path);
              return 0;
            }
        }
      else
        {
          if (fputc ('\n', file) == EOF)
            {
              fclose (file);
              set_error (error_output, error_output_size,
                         "Failed to write JSON export newline to %s",
                         output_path);
              return 0;
            }
        }
    }

  if (fputs ("]\n", file) < 0)
    {
      fclose (file);
      set_error (error_output, error_output_size,
                 "Failed to finalize JSON export file: %s", output_path);
      return 0;
    }

  if (fclose (file) != 0)
    {
      set_error (error_output, error_output_size,
                 "Failed to finalize JSON export file: %s", output_path);
      return 0;
    }

  return 1;
}
