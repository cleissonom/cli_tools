#include "sc_api.h"
#include "sc_config.h"

#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *BLUE = "\033[1;34m";
const char *YELLOW = "\033[1;33m";
const char *GREEN = "\033[1;32m";

enum ApiQuotaStatus
{
  API_QUOTA_OK = 0,
  API_QUOTA_REACHED,
  API_QUOTA_TRACKING_ERROR
};

struct MemoryStruct
{
  char *memory;
  size_t size;
};

int api_quota_reached_error = 0;

static size_t
WriteMemoryCallback (void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc (mem->memory, mem->size + realsize + 1);
  if (ptr == NULL)
    {
      fprintf (stderr, "Not enough memory to reallocate\n");
      return 0;
    }

  mem->memory = ptr;
  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = '\0';

  return realsize;
}

int
get_api_usage_file_path (char *output_path, size_t output_path_size)
{
  const char *custom_file_path = getenv ("SALARY_CALCULATOR_USAGE_FILE");
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

int
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

int
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

int
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

void
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

int
fetch_url (const char *url, struct MemoryStruct *chunk, long *http_code)
{
  CURL *curl_handle = NULL;
  CURLcode res;
  long response_code = 0;
  int global_initialized = 0;
  int success = 0;
  enum ApiQuotaStatus quota_status;

  api_quota_reached_error = 0;

  quota_status = consume_daily_api_quota ();
  if (quota_status == API_QUOTA_REACHED)
    {
      api_quota_reached_error = 1;
      return 0;
    }

  if (quota_status == API_QUOTA_TRACKING_ERROR)
    {
      fprintf (stderr,
               "\033[1;31mFailed to track daily API requests. Aborting to "
               "avoid quota overflow.\033[0m\n");
      return 0;
    }

  chunk->memory = malloc (1);
  if (chunk->memory == NULL)
    {
      fprintf (stderr, "Not enough memory to allocate\n");
      return 0;
    }
  chunk->size = 0;

  if (curl_global_init (CURL_GLOBAL_ALL) != CURLE_OK)
    {
      fprintf (stderr, "curl_global_init() failed\n");
      goto cleanup;
    }
  global_initialized = 1;

  curl_handle = curl_easy_init ();
  if (curl_handle == NULL)
    {
      fprintf (stderr, "curl_easy_init() failed\n");
      goto cleanup;
    }

  curl_easy_setopt (curl_handle, CURLOPT_URL, url);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
  curl_easy_setopt (curl_handle, CURLOPT_USERAGENT, "salary-calculator/1.0");
  curl_easy_setopt (curl_handle, CURLOPT_CONNECTTIMEOUT_MS,
                    REQUEST_CONNECT_TIMEOUT_MS);
  curl_easy_setopt (curl_handle, CURLOPT_TIMEOUT_MS, REQUEST_TIMEOUT_MS);

  res = curl_easy_perform (curl_handle);
  if (res != CURLE_OK)
    {
      fprintf (stderr, "curl_easy_perform() failed: %s\n",
               curl_easy_strerror (res));
      goto cleanup;
    }

  if (curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &response_code)
      != CURLE_OK)
    {
      fprintf (stderr, "Failed to get HTTP status code\n");
      goto cleanup;
    }

  if (response_code < 200 || response_code >= 300)
    {
      fprintf (stderr, "Exchange API returned HTTP status %ld\n", response_code);
      goto cleanup;
    }

  success = 1;

cleanup:
  if (curl_handle != NULL)
    {
      curl_easy_cleanup (curl_handle);
    }
  if (global_initialized)
    {
      curl_global_cleanup ();
    }

  if (!success)
    {
      free (chunk->memory);
      chunk->memory = NULL;
      chunk->size = 0;
    }

  if (http_code != NULL)
    {
      *http_code = response_code;
    }

  return success;
}

enum PairValidationStatus
validate_currency_pair (const char *from_currency, const char *to_currency)
{
  struct MemoryStruct chunk;
  char pair_pattern[(MAX_CURRENCY_CODE * 2) + 4];
  enum PairValidationStatus status = PAIR_CHECK_ERROR;

  if (!fetch_url (API_BASE_URL "/available", &chunk, NULL))
    {
      return PAIR_CHECK_ERROR;
    }

  snprintf (pair_pattern, sizeof (pair_pattern), "\"%s-%s\"", from_currency,
            to_currency);

  if (strstr (chunk.memory, pair_pattern) != NULL)
    {
      status = PAIR_VALID;
    }
  else
    {
      status = PAIR_INVALID;
    }

  free (chunk.memory);
  return status;
}

int
extract_exchange_rate (const char *response_body, double *exchange_rate)
{
  char *start;
  char *endptr;
  double parsed_value;

  start = strstr (response_body, "\"bid\":\"");
  if (start == NULL)
    {
      return 0;
    }
  start += 7;

  errno = 0;
  parsed_value = strtod (start, &endptr);
  if (errno != 0 || endptr == start)
    {
      return 0;
    }

  *exchange_rate = parsed_value;
  return 1;
}

int
get_exchange_rate (const char *from_currency, const char *to_currency,
                   double *exchange_rate)
{
  struct MemoryStruct chunk;
  char url[256];
  int success = 0;

  snprintf (url, sizeof (url), API_BASE_URL "/last/%s-%s", from_currency,
            to_currency);

  if (!fetch_url (url, &chunk, NULL))
    {
      return 0;
    }

  if (!extract_exchange_rate (chunk.memory, exchange_rate))
    {
      fprintf (stderr, "Failed to parse exchange rate from API response\n");
      goto cleanup;
    }

  success = 1;

cleanup:
  free (chunk.memory);
  return success;
}
