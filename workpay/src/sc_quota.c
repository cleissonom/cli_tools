#include "sc_quota.h"

#include "sc_config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int
get_api_usage_file_path (char *output_path, size_t output_path_size)
{
  const char *custom_file_path = getenv ("WORKPAY_USAGE_FILE");
  const char *home_directory;
  int written_chars;

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
                                API_USAGE_FILE_NAME);
      return written_chars > 0 && (size_t)written_chars < output_path_size;
    }

  written_chars = snprintf (output_path, output_path_size, "%s/%s",
                            home_directory, API_USAGE_FILE_NAME);
  return written_chars > 0 && (size_t)written_chars < output_path_size;
}

static int
get_today_date (char *date_output, size_t date_output_size)
{
  time_t now;
  struct tm local_time_buffer;

  now = time (NULL);
  if (now == (time_t)-1)
    {
      return 0;
    }

  if (localtime_r (&now, &local_time_buffer) == NULL)
    {
      return 0;
    }

  return strftime (date_output, date_output_size, "%Y-%m-%d",
                   &local_time_buffer)
         != 0;
}

static int
read_daily_api_usage (const char *usage_file_path, const char *today_date,
                      int *request_count)
{
  FILE *usage_file;
  char file_date[16];
  int file_count;

  usage_file = fopen (usage_file_path, "r");
  if (usage_file == NULL)
    {
      if (errno == ENOENT)
        {
          *request_count = 0;
          return 1;
        }

      fprintf (stderr,
               "\033[1;31mFailed to open API usage file for reading: %s\033"
               "[0m\n",
               usage_file_path);
      return 0;
    }

  if (fscanf (usage_file, "%15s %d", file_date, &file_count) == 2
      && strcmp (file_date, today_date) == 0 && file_count >= 0)
    {
      *request_count = file_count;
    }
  else
    {
      *request_count = 0;
    }

  fclose (usage_file);
  return 1;
}

static int
write_daily_api_usage (const char *usage_file_path, const char *today_date,
                       int request_count)
{
  FILE *usage_file;

  usage_file = fopen (usage_file_path, "w");
  if (usage_file == NULL)
    {
      fprintf (stderr,
               "\033[1;31mFailed to open API usage file for writing: %s\033"
               "[0m\n",
               usage_file_path);
      return 0;
    }

  if (fprintf (usage_file, "%s %d\n", today_date, request_count) < 0)
    {
      fclose (usage_file);
      fprintf (stderr, "\033[1;31mFailed to write API usage file.\033[0m\n");
      return 0;
    }

  fclose (usage_file);
  return 1;
}

static void
print_api_quota_warning_if_needed (int remaining_requests)
{
  if (remaining_requests == 25 || remaining_requests == 10
      || remaining_requests == 5 || remaining_requests == 1)
    {
      fprintf (stderr, "%s%d requests remaining\033[0m\n", YELLOW,
               remaining_requests);
    }
}

enum ApiQuotaStatus
consume_daily_api_quota (void)
{
  char usage_file_path[512];
  char today_date[16];
  int request_count;
  int remaining_requests;

  if (!get_api_usage_file_path (usage_file_path, sizeof (usage_file_path))
      || !get_today_date (today_date, sizeof (today_date)))
    {
      return API_QUOTA_TRACKING_ERROR;
    }

  if (!read_daily_api_usage (usage_file_path, today_date, &request_count))
    {
      return API_QUOTA_TRACKING_ERROR;
    }

  if (request_count >= API_DAILY_REQUEST_LIMIT)
    {
      fprintf (stderr, "\033[1;31mRequests limit reached\033[0m\n");
      return API_QUOTA_REACHED;
    }

  request_count++;

  if (!write_daily_api_usage (usage_file_path, today_date, request_count))
    {
      return API_QUOTA_TRACKING_ERROR;
    }

  remaining_requests = API_DAILY_REQUEST_LIMIT - request_count;
  print_api_quota_warning_if_needed (remaining_requests);

  return API_QUOTA_OK;
}
