#include "sc_api.h"

#include "sc_config.h"
#include "sc_quota.h"

#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MemoryStruct
{
  char *memory;
  size_t size;
};

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

static enum ApiRequestStatus
fetch_url (const char *url, struct MemoryStruct *chunk, long *http_code)
{
  CURL *curl_handle = NULL;
  CURLcode res;
  long response_code = 0;
  int global_initialized = 0;
  int success = 0;
  enum ApiQuotaStatus quota_status;

  quota_status = consume_daily_api_quota ();
  if (quota_status == API_QUOTA_REACHED)
    {
      return API_REQUEST_QUOTA_REACHED;
    }

  if (quota_status == API_QUOTA_TRACKING_ERROR)
    {
      fprintf (stderr,
               "\033[1;31mFailed to track daily API requests. Aborting to "
               "avoid quota overflow.\033[0m\n");
      return API_REQUEST_ERROR;
    }

  chunk->memory = malloc (1);
  if (chunk->memory == NULL)
    {
      fprintf (stderr, "Not enough memory to allocate\n");
      return API_REQUEST_ERROR;
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

  return success ? API_REQUEST_OK : API_REQUEST_ERROR;
}

enum PairValidationStatus
validate_currency_pair (const char *from_currency, const char *to_currency,
                        enum ApiRequestStatus *request_status)
{
  struct MemoryStruct chunk;
  char pair_pattern[(MAX_CURRENCY_CODE * 2) + 4];
  enum PairValidationStatus status = PAIR_CHECK_ERROR;
  enum ApiRequestStatus api_status;

  api_status = fetch_url (API_BASE_URL "/available", &chunk, NULL);
  if (api_status != API_REQUEST_OK)
    {
      if (request_status != NULL)
        {
          *request_status = api_status;
        }
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

  if (request_status != NULL)
    {
      *request_status = API_REQUEST_OK;
    }

  return status;
}

static int
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

enum ApiRequestStatus
get_exchange_rate (const char *from_currency, const char *to_currency,
                   double *exchange_rate)
{
  struct MemoryStruct chunk;
  char url[256];
  enum ApiRequestStatus api_status;

  snprintf (url, sizeof (url), API_BASE_URL "/last/%s-%s", from_currency,
            to_currency);

  api_status = fetch_url (url, &chunk, NULL);
  if (api_status != API_REQUEST_OK)
    {
      return api_status;
    }

  if (!extract_exchange_rate (chunk.memory, exchange_rate))
    {
      fprintf (stderr, "Failed to parse exchange rate from API response\n");
      free (chunk.memory);
      return API_REQUEST_ERROR;
    }

  free (chunk.memory);
  return API_REQUEST_OK;
}
