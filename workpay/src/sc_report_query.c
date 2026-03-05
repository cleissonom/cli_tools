#include "sc_report_query.h"

#include "sc_parse.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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
report_filter_init (struct ReportFilter *filter)
{
  if (filter == NULL)
    {
      return;
    }

  filter->from_date = NULL;
  filter->to_date = NULL;
  filter->has_from_currency = 0;
  filter->from_currency[0] = '\0';
  filter->has_to_currency = 0;
  filter->to_currency[0] = '\0';
  filter->has_limit = 0;
  filter->limit = 0;
}

static int
compare_date_parts (int year_a, int month_a, int day_a, int year_b, int month_b,
                    int day_b)
{
  if (year_a != year_b)
    {
      return year_a < year_b ? -1 : 1;
    }
  if (month_a != month_b)
    {
      return month_a < month_b ? -1 : 1;
    }
  if (day_a != day_b)
    {
      return day_a < day_b ? -1 : 1;
    }

  return 0;
}

int
report_validate_filter (const struct ReportFilter *filter, char *error_output,
                        size_t error_output_size)
{
  int from_year;
  int from_month;
  int from_day;
  int to_year;
  int to_month;
  int to_day;

  if (filter == NULL)
    {
      set_error (error_output, error_output_size,
                 "Internal error: missing report filter.");
      return 0;
    }

  if (filter->has_limit && filter->limit <= 0)
    {
      set_error (error_output, error_output_size,
                 "Invalid --limit. Use a positive integer.");
      return 0;
    }

  if (filter->from_date != NULL
      && !parse_date_ymd (filter->from_date, &from_year, &from_month, &from_day))
    {
      set_error (error_output, error_output_size,
                 "Invalid --from-date. Use YYYY-MM-DD.");
      return 0;
    }

  if (filter->to_date != NULL
      && !parse_date_ymd (filter->to_date, &to_year, &to_month, &to_day))
    {
      set_error (error_output, error_output_size,
                 "Invalid --to-date. Use YYYY-MM-DD.");
      return 0;
    }

  if (filter->from_date != NULL && filter->to_date != NULL
      && compare_date_parts (from_year, from_month, from_day, to_year, to_month,
                             to_day)
             > 0)
    {
      set_error (error_output, error_output_size,
                 "Invalid date range: --from-date is after --to-date.");
      return 0;
    }

  return 1;
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

static int
entry_matches_filter (const struct ReportEntry *entry,
                      const struct ReportFilter *filter)
{
  int entry_year;
  int entry_month;
  int entry_day;
  int filter_year;
  int filter_month;
  int filter_day;
  char entry_date[REPORT_DATE_SIZE];

  if (filter->has_from_currency
      && strcmp (entry->from_currency, filter->from_currency) != 0)
    {
      return 0;
    }

  if (filter->has_to_currency && strcmp (entry->to_currency, filter->to_currency) != 0)
    {
      return 0;
    }

  snprintf (entry_date, sizeof (entry_date), "%.10s", entry->created_at);
  if (!parse_date_ymd (entry_date, &entry_year, &entry_month, &entry_day))
    {
      return 0;
    }

  if (filter->from_date != NULL)
    {
      parse_date_ymd (filter->from_date, &filter_year, &filter_month, &filter_day);
      if (compare_date_parts (entry_year, entry_month, entry_day, filter_year,
                              filter_month, filter_day)
          < 0)
        {
          return 0;
        }
    }

  if (filter->to_date != NULL)
    {
      parse_date_ymd (filter->to_date, &filter_year, &filter_month, &filter_day);
      if (compare_date_parts (entry_year, entry_month, entry_day, filter_year,
                              filter_month, filter_day)
          > 0)
        {
          return 0;
        }
    }

  return 1;
}

int
report_filter_entries (const struct ReportEntry *entries, size_t entry_count,
                       const struct ReportFilter *filter,
                       struct ReportEntryList *filtered_entries,
                       char *error_output, size_t error_output_size)
{
  size_t index;

  if (filtered_entries == NULL)
    {
      set_error (error_output, error_output_size,
                 "Internal error: missing filtered entries output.");
      return 0;
    }

  filtered_entries->items = NULL;
  filtered_entries->count = 0;
  filtered_entries->capacity = 0;

  if (!report_validate_filter (filter, error_output, error_output_size))
    {
      return 0;
    }

  for (index = 0; index < entry_count; index++)
    {
      if (!entry_matches_filter (&entries[index], filter))
        {
          continue;
        }

      if (!append_entry_to_list (filtered_entries, &entries[index]))
        {
          set_error (error_output, error_output_size,
                     "Out of memory while filtering report entries.");
          free (filtered_entries->items);
          filtered_entries->items = NULL;
          filtered_entries->count = 0;
          filtered_entries->capacity = 0;
          return 0;
        }

      if (filter->has_limit && (int)filtered_entries->count >= filter->limit)
        {
          break;
        }
    }

  return 1;
}

static int
append_group (struct ReportSummary *summary, const char *from_currency,
              const char *to_currency, size_t *group_index)
{
  struct ReportSummaryGroup *new_groups;
  size_t new_capacity;

  if (summary->group_count == summary->group_capacity)
    {
      new_capacity = summary->group_capacity == 0 ? 8 : summary->group_capacity * 2;
      new_groups = realloc (summary->groups,
                            new_capacity * sizeof (struct ReportSummaryGroup));
      if (new_groups == NULL)
        {
          return 0;
        }

      summary->groups = new_groups;
      summary->group_capacity = new_capacity;
    }

  *group_index = summary->group_count;
  summary->group_count++;

  snprintf (summary->groups[*group_index].from_currency,
            sizeof (summary->groups[*group_index].from_currency), "%s",
            from_currency);
  snprintf (summary->groups[*group_index].to_currency,
            sizeof (summary->groups[*group_index].to_currency), "%s", to_currency);
  summary->groups[*group_index].count = 0;
  summary->groups[*group_index].total_from_currency = 0.0;
  summary->groups[*group_index].total_to_currency = 0.0;
  summary->groups[*group_index].exchange_rate_sum = 0.0;

  return 1;
}

static int
find_group_index (const struct ReportSummary *summary, const char *from_currency,
                  const char *to_currency, size_t *group_index)
{
  size_t index;

  for (index = 0; index < summary->group_count; index++)
    {
      if (strcmp (summary->groups[index].from_currency, from_currency) == 0
          && strcmp (summary->groups[index].to_currency, to_currency) == 0)
        {
          *group_index = index;
          return 1;
        }
    }

  return 0;
}

void
report_summary_free (struct ReportSummary *summary)
{
  if (summary == NULL)
    {
      return;
    }

  free (summary->groups);
  summary->groups = NULL;
  summary->group_count = 0;
  summary->group_capacity = 0;
  summary->total_entries = 0;
}

int
report_build_summary (const struct ReportEntry *entries, size_t entry_count,
                      struct ReportSummary *summary, char *error_output,
                      size_t error_output_size)
{
  size_t index;

  if (summary == NULL)
    {
      set_error (error_output, error_output_size,
                 "Internal error: missing report summary output.");
      return 0;
    }

  summary->groups = NULL;
  summary->group_count = 0;
  summary->group_capacity = 0;
  summary->total_entries = entry_count;

  for (index = 0; index < entry_count; index++)
    {
      size_t group_index;

      if (!find_group_index (summary, entries[index].from_currency,
                             entries[index].to_currency, &group_index))
        {
          if (!append_group (summary, entries[index].from_currency,
                             entries[index].to_currency, &group_index))
            {
              report_summary_free (summary);
              set_error (error_output, error_output_size,
                         "Out of memory while building summary.");
              return 0;
            }
        }

      summary->groups[group_index].count++;
      summary->groups[group_index].total_from_currency
          += entries[index].total_from_currency;
      summary->groups[group_index].total_to_currency += entries[index].total_to_currency;
      summary->groups[group_index].exchange_rate_sum += entries[index].exchange_rate;
    }

  return 1;
}
