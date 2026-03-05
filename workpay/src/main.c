#include "sc_api.h"
#include "sc_cli.h"
#include "sc_config.h"
#include "sc_money.h"
#include "sc_parse.h"
#include "sc_report_query.h"
#include "sc_report_render.h"
#include "sc_report_store.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static int
resolve_save_time (const char *save_date, time_t *resolved_time,
                   struct tm *resolved_local_time)
{
  if (save_date == NULL)
    {
      *resolved_time = time (NULL);
      if (*resolved_time == (time_t)-1
          || localtime_r (resolved_time, resolved_local_time) == NULL)
        {
          return 0;
        }

      return 1;
    }

  {
    int year;
    int month;
    int day;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    struct tm manual_time;
    char date_part[11];

    if (strlen (save_date) == 10)
      {
        if (!parse_date_ymd (save_date, &year, &month, &day))
          {
            return 0;
          }
      }
    else if (strlen (save_date) == 19 && save_date[10] == 'T')
      {
        snprintf (date_part, sizeof (date_part), "%.10s", save_date);
        if (!parse_date_ymd (date_part, &year, &month, &day)
            || !parse_time (save_date + 11, &hours, &minutes, &seconds))
          {
            return 0;
          }
      }
    else
      {
        return 0;
      }

    memset (&manual_time, 0, sizeof (manual_time));
    manual_time.tm_year = year - 1900;
    manual_time.tm_mon = month - 1;
    manual_time.tm_mday = day;
    manual_time.tm_hour = hours;
    manual_time.tm_min = minutes;
    manual_time.tm_sec = seconds;
    manual_time.tm_isdst = -1;

    *resolved_time = mktime (&manual_time);
    if (*resolved_time == (time_t)-1)
      {
        return 0;
      }

    if (localtime_r (resolved_time, resolved_local_time) == NULL)
      {
        return 0;
      }

    return 1;
  }
}

static int
run_calculation_mode (const struct CliArguments *arguments)
{
  enum PairValidationStatus pair_status;
  enum ApiRequestStatus api_status;
  double hourly_rate;
  int hours;
  int minutes;
  int seconds;
  double base_earned_from_currency;
  double extra_from_amount = 0.0;
  double total_earned_from_currency;
  double exchange_rate;
  double total_earned_to_currency;
  char formatted_extra_from_currency[128];
  char formatted_total_from_currency[128];
  char formatted_total_to_currency[128];

  if (!parse_hourly_rate (arguments->hourly_rate_arg, &hourly_rate))
    {
      fprintf (stderr,
               "\033[1;31mInvalid hourly rate. Use a non-negative number.\033"
               "[0m\n");
      return 1;
    }

  if (!parse_time (arguments->time_arg, &hours, &minutes, &seconds))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid time format. Use HH:MM:SS with MM/SS between 00 "
          "and 59.\033[0m\n");
      return 1;
    }

  if (arguments->has_extra_from_amount
      && (!parse_hourly_rate (arguments->extra_from_amount, &extra_from_amount)
          || extra_from_amount < 0.0))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid value for --extra-from. Use a non-negative "
          "number.\033[0m\n");
      return 1;
    }

  pair_status = validate_currency_pair (arguments->from_currency,
                                        arguments->to_currency, &api_status);
  if (pair_status == PAIR_CHECK_ERROR)
    {
      if (api_status == API_REQUEST_QUOTA_REACHED)
        {
          return 1;
        }
      fprintf (stderr,
               "\033[1;31mFailed to validate currency pair against "
               "AwesomeAPI.\033[0m\n");
      return 1;
    }

  if (pair_status == PAIR_INVALID)
    {
      fprintf (stderr, "\033[1;31mUnsupported currency pair: %s-%s\033[0m\n",
               arguments->from_currency, arguments->to_currency);
      return 1;
    }

  base_earned_from_currency = calculate_payment (hours, minutes, seconds,
                                                 hourly_rate);
  total_earned_from_currency = base_earned_from_currency + extra_from_amount;

  if (arguments->has_exchange_rate)
    {
      if (!parse_hourly_rate (arguments->exchange_rate, &exchange_rate)
          || exchange_rate <= 0.0)
        {
          fprintf (stderr,
                   "\033[1;31mInvalid value for --exchange-rate. Use a "
                   "positive number.\033[0m\n");
          return 1;
        }
    }
  else
    {
      api_status = get_exchange_rate (arguments->from_currency,
                                      arguments->to_currency, &exchange_rate);
      if (api_status != API_REQUEST_OK)
        {
          if (api_status == API_REQUEST_QUOTA_REACHED)
            {
              return 1;
            }
          fprintf (stderr,
                   "\033[1;31mFailed to retrieve exchange rate.\033[0m\n");
          return 1;
        }
    }

  total_earned_to_currency = total_earned_from_currency * exchange_rate;

  if ((extra_from_amount > 0.0
       && !format_money (formatted_extra_from_currency,
                         sizeof (formatted_extra_from_currency),
                         extra_from_amount, arguments->from_currency, 2))
      || !format_money (formatted_total_from_currency,
                     sizeof (formatted_total_from_currency),
                     total_earned_from_currency, arguments->from_currency, 2)
      || !format_money (formatted_total_to_currency,
                        sizeof (formatted_total_to_currency),
                        total_earned_to_currency, arguments->to_currency, 2))
    {
      fprintf (stderr, "\033[1;31mFailed to format currency values.\033[0m\n");
      return 1;
    }

  printf ("%sTotal hours worked:\033[0m %.2f hours\n", BLUE,
          (double)hours + (minutes / 60.0) + (seconds / 3600.0));
  printf ("%sHourly wage (%s):\033[0m %.6f %s\n", BLUE,
          arguments->from_currency, hourly_rate, arguments->from_currency);
  if (extra_from_amount > 0.0)
    {
      printf ("%sExtra amount in %s:\033[0m %s\n", BLUE, arguments->from_currency,
              formatted_extra_from_currency);
    }
  printf ("%sExchange rate (%s -> %s):\033[0m %.6f\n", YELLOW,
          arguments->from_currency, arguments->to_currency, exchange_rate);
  printf ("%sTotal earned in %s:\033[0m %s\n", YELLOW,
          arguments->from_currency, formatted_total_from_currency);
  printf ("%sTotal earned in %s:\033[0m %s\n", GREEN, arguments->to_currency,
          formatted_total_to_currency);

  if (arguments->save_entry)
    {
      struct ReportEntry entry;
      char error_message[256];
      char entries_file_path[512];
      time_t save_time;
      struct tm local_time_buffer;

      if (arguments->tag != NULL && strlen (arguments->tag) >= REPORT_TAG_SIZE)
        {
          fprintf (stderr,
                   "\033[1;31mTag is too long. Maximum %d characters.\033[0m\n",
                   REPORT_TAG_SIZE - 1);
          return 1;
        }

      memset (&entry, 0, sizeof (entry));

      if (!resolve_save_time (arguments->save_date, &save_time,
                              &local_time_buffer)
          || strftime (entry.created_at, sizeof (entry.created_at),
                       "%Y-%m-%dT%H:%M:%S", &local_time_buffer)
                 == 0)
        {
          fprintf (stderr,
                   "\033[1;31mFailed to generate entry timestamp.\033[0m\n");
          return 1;
        }

      entry.entry_id = (long long)save_time;
      snprintf (entry.from_currency, sizeof (entry.from_currency), "%s",
                arguments->from_currency);
      snprintf (entry.to_currency, sizeof (entry.to_currency), "%s",
                arguments->to_currency);
      entry.hourly_rate = hourly_rate;
      entry.hours = hours;
      entry.minutes = minutes;
      entry.seconds = seconds;
      entry.total_from_currency = total_earned_from_currency;
      entry.exchange_rate = exchange_rate;
      entry.total_to_currency = total_earned_to_currency;
      snprintf (entry.tag, sizeof (entry.tag), "%s",
                arguments->tag != NULL ? arguments->tag : "");

      if (report_store_append_entry (&entry, error_message,
                                     sizeof (error_message))
          != REPORT_STORE_OK)
        {
          fprintf (stderr, "\033[1;31m%s\033[0m\n", error_message);
          return 1;
        }

      if (report_store_get_entries_file_path (entries_file_path,
                                              sizeof (entries_file_path)))
        {
          printf ("Saved entry %lld to %s\n", entry.entry_id, entries_file_path);
        }
      else
        {
          printf ("Saved entry %lld\n", entry.entry_id);
        }
    }

  return 0;
}

static int
run_report_mode (const struct CliArguments *arguments)
{
  struct ReportEntryList loaded_entries;
  struct ReportEntryList filtered_entries;
  struct ReportFilter filter;
  char error_message[256];

  if (report_store_load_entries (&loaded_entries, error_message,
                                 sizeof (error_message))
      != REPORT_STORE_OK)
    {
      fprintf (stderr, "\033[1;31m%s\033[0m\n", error_message);
      return 1;
    }

  report_filter_init (&filter);
  filter.from_date = arguments->from_date;
  filter.to_date = arguments->to_date;
  filter.has_from_currency = arguments->has_from_option;
  filter.has_to_currency = arguments->has_to_option;
  if (filter.has_from_currency)
    {
      snprintf (filter.from_currency, sizeof (filter.from_currency), "%s",
                arguments->from_currency);
    }
  if (filter.has_to_currency)
    {
      snprintf (filter.to_currency, sizeof (filter.to_currency), "%s",
                arguments->to_currency);
    }

  if (arguments->command_type == CLI_COMMAND_REPORT_LIST && arguments->has_limit)
    {
      filter.has_limit = 1;
      filter.limit = arguments->limit;
    }

  if (!report_filter_entries (loaded_entries.items, loaded_entries.count, &filter,
                              &filtered_entries, error_message,
                              sizeof (error_message)))
    {
      report_entry_list_free (&loaded_entries);
      fprintf (stderr, "\033[1;31m%s\033[0m\n", error_message);
      return 1;
    }

  report_entry_list_free (&loaded_entries);

  if (filtered_entries.count == 0)
    {
      report_entry_list_free (&filtered_entries);
      printf ("No entries found for the selected filters.\n");
      return 0;
    }

  if (arguments->command_type == CLI_COMMAND_REPORT_SUMMARY)
    {
      struct ReportSummary summary;

      if (!report_build_summary (filtered_entries.items, filtered_entries.count,
                                 &summary, error_message,
                                 sizeof (error_message)))
        {
          report_entry_list_free (&filtered_entries);
          fprintf (stderr, "\033[1;31m%s\033[0m\n", error_message);
          return 1;
        }

      report_render_summary (stdout, &summary);
      report_summary_free (&summary);
      report_entry_list_free (&filtered_entries);
      return 0;
    }

  if (arguments->command_type == CLI_COMMAND_REPORT_LIST)
    {
      report_render_list (stdout, filtered_entries.items, filtered_entries.count);
      report_entry_list_free (&filtered_entries);
      return 0;
    }

  if (arguments->command_type == CLI_COMMAND_REPORT_EXPORT)
    {
      int export_ok;
      size_t exported_count = filtered_entries.count;

      if (arguments->export_format == REPORT_EXPORT_FORMAT_CSV)
        {
          export_ok = report_export_entries_csv (arguments->output_path,
                                                 filtered_entries.items,
                                                 filtered_entries.count,
                                                 error_message,
                                                 sizeof (error_message));
        }
      else
        {
          export_ok = report_export_entries_json (arguments->output_path,
                                                  filtered_entries.items,
                                                  filtered_entries.count,
                                                  error_message,
                                                  sizeof (error_message));
        }

      report_entry_list_free (&filtered_entries);

      if (!export_ok)
        {
          fprintf (stderr, "\033[1;31m%s\033[0m\n", error_message);
          return 1;
        }

      printf ("Exported %zu entries to %s\n", exported_count,
              arguments->output_path);
      return 0;
    }

  report_entry_list_free (&filtered_entries);
  fprintf (stderr, "\033[1;31mUnsupported command mode.\033[0m\n");
  return 1;
}

int
main (int argc, char *argv[])
{
  struct CliArguments arguments;
  enum CliParseStatus cli_status;

  cli_status = parse_cli_arguments (argc, argv, &arguments, argv[0]);
  if (cli_status == CLI_PARSE_SHOW_HELP)
    {
      print_help (argv[0]);
      return 0;
    }

  if (cli_status == CLI_PARSE_ERROR)
    {
      return 1;
    }

  if (arguments.command_type == CLI_COMMAND_CALCULATION)
    {
      return run_calculation_mode (&arguments);
    }

  return run_report_mode (&arguments);
}
