#include <curl/curl.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define API_BASE_URL "https://economia.awesomeapi.com.br/json"
#define DEFAULT_FROM "USD"
#define DEFAULT_TO "BRL"
#define MAX_CURRENCY_CODE 16
#define REQUEST_CONNECT_TIMEOUT_MS 5000L
#define REQUEST_TIMEOUT_MS 10000L
#define API_DAILY_REQUEST_LIMIT 100
#define API_USAGE_FILE_NAME ".salary_calculator_api_usage"

const char *BLUE = "\033[1;34m";
const char *YELLOW = "\033[1;33m";
const char *GREEN = "\033[1;32m";

enum CliParseStatus
{
  CLI_PARSE_OK = 0,
  CLI_PARSE_SHOW_HELP,
  CLI_PARSE_ERROR
};

enum PairValidationStatus
{
  PAIR_VALID = 0,
  PAIR_INVALID,
  PAIR_CHECK_ERROR
};

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

struct CurrencyFormat
{
  const char *code;
  const char *symbol;
  char decimal_separator;
  char thousands_separator;
  int space_after_symbol;
};

static const struct CurrencyFormat CURRENCY_FORMATS[] = {
  { "USD", "$", '.', ',', 0 },      { "BRL", "R$", ',', '.', 1 },
  { "BRLT", "R$", ',', '.', 1 },    { "BRLPTAX", "R$", ',', '.', 1 },
  { "EUR", "€", ',', '.', 1 },      { "GBP", "£", '.', ',', 0 },
  { "JPY", "¥", '.', ',', 0 },      { "CAD", "C$", '.', ',', 0 },
  { "AUD", "A$", '.', ',', 0 },     { "NZD", "NZ$", '.', ',', 0 },
  { "CHF", "CHF", '.', ',', 1 },    { "CNY", "¥", '.', ',', 0 },
  { "HKD", "HK$", '.', ',', 0 },    { "SGD", "S$", '.', ',', 0 },
  { "MXN", "MX$", '.', ',', 0 },    { "ARS", "AR$", ',', '.', 0 },
  { "CLP", "CLP$", ',', '.', 0 },   { "COP", "COP$", ',', '.', 0 },
  { "PEN", "S/", ',', '.', 0 },     { "PYG", "₲", ',', '.', 0 },
  { "UYU", "$U", ',', '.', 0 },     { "BOB", "Bs", ',', '.', 1 },
  { "RUB", "₽", ',', '.', 0 },      { "TRY", "₺", ',', '.', 0 },
  { "PLN", "zł", ',', '.', 0 },     { "SEK", "kr", ',', '.', 1 },
  { "NOK", "kr", ',', '.', 1 },     { "DKK", "kr", ',', '.', 1 },
  { "CZK", "Kč", ',', '.', 1 },     { "HUF", "Ft", ',', '.', 1 },
  { "INR", "₹", '.', ',', 0 },      { "KRW", "₩", '.', ',', 0 },
  { "TWD", "NT$", '.', ',', 0 },    { "THB", "฿", '.', ',', 0 },
  { "ILS", "₪", '.', ',', 0 },      { "AED", "AED", '.', ',', 1 },
  { "SAR", "SAR", '.', ',', 1 },    { "QAR", "QAR", '.', ',', 1 },
  { "KWD", "KWD", '.', ',', 1 },    { "JOD", "JOD", '.', ',', 1 },
  { "BTC", "BTC", '.', ',', 1 },    { "ETH", "ETH", '.', ',', 1 },
  { "LTC", "LTC", '.', ',', 1 },    { "XRP", "XRP", '.', ',', 1 },
  { "DOGE", "DOGE", '.', ',', 1 },  { "SOL", "SOL", '.', ',', 1 },
  { "BNB", "BNB", '.', ',', 1 },    { "BRETT", "BRETT", '.', ',', 1 },
  { "XAU", "XAU", '.', ',', 1 },    { "XAG", "XAG", '.', ',', 1 },
  { "XAGG", "XAGG", '.', ',', 1 },  { "XBR", "XBR", '.', ',', 1 }
};

static int api_quota_reached_error = 0;

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

void
print_usage (FILE *stream, const char *program_name)
{
  fprintf (stream,
           "Usage: %s [--from CODE] [--to CODE] [--help] <hourly_rate> "
           "<hours:minutes:seconds>\n",
           program_name);
}

void
print_help (const char *program_name)
{
  print_usage (stdout, program_name);
  printf ("\n");
  printf ("Calculate earned amount based on worked time and hourly rate.\n");
  printf ("\n");
  printf ("Arguments:\n");
  printf ("  <hourly_rate>            Non-negative decimal number.\n");
  printf ("  <hours:minutes:seconds>  Time worked in HH:MM:SS format.\n");
  printf ("\n");
  printf ("Flags:\n");
  printf ("  -h, --help               Show this help message.\n");
  printf ("  --from CODE              Source currency (default: %s).\n",
          DEFAULT_FROM);
  printf ("  --to CODE                Target currency (default: %s).\n",
          DEFAULT_TO);
  printf ("\n");
  printf ("Currency pairs are validated against AwesomeAPI supported pairs.\n");
  printf ("Daily API limit: %d requests/day.\n", API_DAILY_REQUEST_LIMIT);
  printf ("Warnings are shown when 25, 10, 5, and 1 requests remain.\n");
  printf ("When the limit is reached, execution stops.\n");
  printf ("Reference list: %s/available\n", API_BASE_URL);
  printf ("\n");
  printf ("Examples:\n");
  printf ("  %s 25 08:30:45\n", program_name);
  printf ("  %s --from EUR --to USD 40 07:15:00\n", program_name);
  printf ("  %s --from BTC --to BRL 0.005 01:00:00\n", program_name);
}

const struct CurrencyFormat *
find_currency_format (const char *currency_code)
{
  size_t index;

  for (index = 0; index < sizeof (CURRENCY_FORMATS) / sizeof (CURRENCY_FORMATS[0]);
       index++)
    {
      if (strcmp (CURRENCY_FORMATS[index].code, currency_code) == 0)
        {
          return &CURRENCY_FORMATS[index];
        }
    }

  return NULL;
}

int
format_grouped_integer (unsigned long long value, char thousands_separator,
                        char *output, size_t output_size)
{
  char digits[32];
  size_t digits_length;
  size_t output_length;
  size_t source_index;
  size_t destination_index;
  int group_count = 0;

  snprintf (digits, sizeof (digits), "%llu", value);
  digits_length = strlen (digits);
  output_length = digits_length + ((digits_length - 1) / 3);

  if (output_length + 1 > output_size)
    {
      return 0;
    }

  output[output_length] = '\0';

  source_index = digits_length;
  destination_index = output_length;

  while (source_index > 0)
    {
      output[--destination_index] = digits[--source_index];
      group_count++;

      if (group_count == 3 && source_index > 0)
        {
          output[--destination_index] = thousands_separator;
          group_count = 0;
        }
    }

  return 1;
}

int
format_money (char *output, size_t output_size, double amount,
              const char *currency_code, int decimal_places)
{
  const struct CurrencyFormat *format;
  char integer_part_buffer[64];
  char decimal_part_buffer[32];
  unsigned long long scaled_value;
  unsigned long long integer_part;
  unsigned long long decimal_part;
  unsigned long long decimal_base = 1;
  double absolute_amount;
  int is_negative;
  int index;
  int decimal_result;
  int written_chars;

  if (decimal_places < 0 || decimal_places > 9)
    {
      return 0;
    }

  format = find_currency_format (currency_code);
  is_negative = amount < 0.0;
  absolute_amount = fabs (amount);

  for (index = 0; index < decimal_places; index++)
    {
      decimal_base *= 10ULL;
    }

  scaled_value = (unsigned long long)llround (absolute_amount * decimal_base);
  integer_part = scaled_value / decimal_base;
  decimal_part = scaled_value % decimal_base;

  if (!format_grouped_integer (
          integer_part,
          format != NULL ? format->thousands_separator : ',',
          integer_part_buffer, sizeof (integer_part_buffer)))
    {
      return 0;
    }

  if (decimal_places > 0)
    {
      decimal_result = snprintf (decimal_part_buffer, sizeof (decimal_part_buffer),
                                 "%0*llu", decimal_places, decimal_part);
      if (decimal_result <= 0
          || (size_t)decimal_result >= sizeof (decimal_part_buffer))
        {
          return 0;
        }
    }
  else
    {
      decimal_part_buffer[0] = '\0';
    }

  if (format != NULL)
    {
      if (decimal_places > 0)
        {
          written_chars = snprintf (
              output, output_size, "%s%s%s%s%c%s",
              is_negative ? "-" : "", format->symbol,
              format->space_after_symbol ? " " : "", integer_part_buffer,
              format->decimal_separator, decimal_part_buffer);
        }
      else
        {
          written_chars = snprintf (output, output_size, "%s%s%s%s",
                                    is_negative ? "-" : "", format->symbol,
                                    format->space_after_symbol ? " " : "",
                                    integer_part_buffer);
        }
    }
  else
    {
      if (decimal_places > 0)
        {
          written_chars
              = snprintf (output, output_size, "%s%s%c%s %s",
                          is_negative ? "-" : "", integer_part_buffer, '.',
                          decimal_part_buffer, currency_code);
        }
      else
        {
          written_chars = snprintf (output, output_size, "%s%s %s",
                                    is_negative ? "-" : "", integer_part_buffer,
                                    currency_code);
        }
    }

  if (written_chars <= 0 || (size_t)written_chars >= output_size)
    {
      return 0;
    }

  return 1;
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
normalize_currency_code (const char *input, char *output, size_t output_size)
{
  size_t i;
  size_t input_length;

  if (input == NULL || output == NULL || output_size < 2)
    {
      return 0;
    }

  input_length = strlen (input);
  if (input_length < 3 || input_length >= output_size)
    {
      return 0;
    }

  for (i = 0; i < input_length; i++)
    {
      unsigned char ch = (unsigned char)input[i];
      if (!isalnum (ch))
        {
          return 0;
        }
      output[i] = (char)toupper (ch);
    }

  output[input_length] = '\0';
  return 1;
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

double
calculate_payment (int hours, int minutes, int seconds, double hourly_rate)
{
  double total_hours = hours + (minutes / 60.0) + (seconds / 3600.0);
  return total_hours * hourly_rate;
}

int
parse_time (const char *time_str, int *hours, int *minutes, int *seconds)
{
  char extra_char;
  int parsed_items
      = sscanf (time_str, "%d:%d:%d%c", hours, minutes, seconds, &extra_char);

  if (parsed_items != 3)
    {
      return 0;
    }

  if (*hours < 0 || *minutes < 0 || *minutes > 59 || *seconds < 0
      || *seconds > 59)
    {
      return 0;
    }

  return 1;
}

int
parse_hourly_rate (const char *hourly_rate_str, double *hourly_rate)
{
  char *endptr;
  double parsed_rate;

  errno = 0;
  parsed_rate = strtod (hourly_rate_str, &endptr);

  if (errno != 0 || endptr == hourly_rate_str || *endptr != '\0'
      || parsed_rate < 0.0)
    {
      return 0;
    }

  *hourly_rate = parsed_rate;
  return 1;
}

int
parse_currency_value (const char *option_name, const char *value,
                      char *output_currency, size_t output_size)
{
  if (!normalize_currency_code (value, output_currency, output_size))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid value for %s. Use only letters and digits "
          "(example: USD, BRL, BRLT, BTC).\033[0m\n",
          option_name);
      return 0;
    }

  return 1;
}

enum CliParseStatus
parse_cli_arguments (int argc, char *argv[], char *from_currency,
                     size_t from_size, char *to_currency, size_t to_size,
                     const char **hourly_rate_arg, const char **time_arg,
                     const char *program_name)
{
  int index;
  int positional_count = 0;

  snprintf (from_currency, from_size, "%s", DEFAULT_FROM);
  snprintf (to_currency, to_size, "%s", DEFAULT_TO);

  *hourly_rate_arg = NULL;
  *time_arg = NULL;

  for (index = 1; index < argc; index++)
    {
      const char *argument = argv[index];

      if (strcmp (argument, "-h") == 0 || strcmp (argument, "--help") == 0)
        {
          return CLI_PARSE_SHOW_HELP;
        }

      if (strcmp (argument, "--from") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr,
                       "\033[1;31mMissing value for --from option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          if (!parse_currency_value ("--from", argv[++index], from_currency,
                                     from_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--from=", 7) == 0)
        {
          if (!parse_currency_value ("--from", argument + 7, from_currency,
                                     from_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strcmp (argument, "--to") == 0)
        {
          if (index + 1 >= argc)
            {
              fprintf (stderr,
                       "\033[1;31mMissing value for --to option.\033[0m\n");
              return CLI_PARSE_ERROR;
            }
          if (!parse_currency_value ("--to", argv[++index], to_currency,
                                     to_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--to=", 5) == 0)
        {
          if (!parse_currency_value ("--to", argument + 5, to_currency,
                                     to_size))
            {
              return CLI_PARSE_ERROR;
            }
          continue;
        }

      if (strncmp (argument, "--", 2) == 0)
        {
          fprintf (stderr, "\033[1;31mUnknown option: %s\033[0m\n", argument);
          return CLI_PARSE_ERROR;
        }

      if (positional_count == 0)
        {
          *hourly_rate_arg = argument;
          positional_count++;
          continue;
        }

      if (positional_count == 1)
        {
          *time_arg = argument;
          positional_count++;
          continue;
        }

      fprintf (stderr, "\033[1;31mToo many positional arguments.\033[0m\n");
      return CLI_PARSE_ERROR;
    }

  if (positional_count != 2)
    {
      fprintf (stderr, "\033[1;31m");
      print_usage (stderr, program_name);
      fprintf (stderr, "\033[0m");
      return CLI_PARSE_ERROR;
    }

  return CLI_PARSE_OK;
}

int
main (int argc, char *argv[])
{
  const char *hourly_rate_arg;
  const char *time_arg;
  char from_currency[MAX_CURRENCY_CODE];
  char to_currency[MAX_CURRENCY_CODE];
  enum CliParseStatus cli_status;
  enum PairValidationStatus pair_status;
  double hourly_rate;
  int hours, minutes, seconds;
  double total_earned_from_currency;
  double exchange_rate;
  double total_earned_to_currency;
  char formatted_total_from_currency[128];
  char formatted_total_to_currency[128];

  cli_status = parse_cli_arguments (argc, argv, from_currency,
                                    sizeof (from_currency), to_currency,
                                    sizeof (to_currency), &hourly_rate_arg,
                                    &time_arg, argv[0]);
  if (cli_status == CLI_PARSE_SHOW_HELP)
    {
      print_help (argv[0]);
      return 0;
    }

  if (cli_status == CLI_PARSE_ERROR)
    {
      return 1;
    }

  if (!parse_hourly_rate (hourly_rate_arg, &hourly_rate))
    {
      fprintf (stderr,
               "\033[1;31mInvalid hourly rate. Use a non-negative number.\033"
               "[0m\n");
      return 1;
    }

  if (!parse_time (time_arg, &hours, &minutes, &seconds))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid time format. Use HH:MM:SS with MM/SS between 00 "
          "and 59.\033[0m\n");
      return 1;
    }

  pair_status = validate_currency_pair (from_currency, to_currency);
  if (pair_status == PAIR_CHECK_ERROR)
    {
      if (api_quota_reached_error)
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
               from_currency, to_currency);
      return 1;
    }

  total_earned_from_currency
      = calculate_payment (hours, minutes, seconds, hourly_rate);

  if (!get_exchange_rate (from_currency, to_currency, &exchange_rate))
    {
      if (api_quota_reached_error)
        {
          return 1;
        }
      fprintf (stderr, "\033[1;31mFailed to retrieve exchange rate.\033[0m\n");
      return 1;
    }

  total_earned_to_currency = total_earned_from_currency * exchange_rate;

  if (!format_money (formatted_total_from_currency,
                     sizeof (formatted_total_from_currency),
                     total_earned_from_currency, from_currency, 2)
      || !format_money (formatted_total_to_currency,
                        sizeof (formatted_total_to_currency),
                        total_earned_to_currency, to_currency, 2))
    {
      fprintf (stderr, "\033[1;31mFailed to format currency values.\033[0m\n");
      return 1;
    }

  printf ("%sTotal hours worked:\033[0m %.2f hours\n", BLUE,
          (double)hours + (minutes / 60.0) + (seconds / 3600.0));
  printf ("%sHourly wage (%s):\033[0m %.6f %s\n", BLUE, from_currency,
          hourly_rate, from_currency);
  printf ("%sExchange rate (%s -> %s):\033[0m %.6f\n", YELLOW, from_currency,
          to_currency, exchange_rate);
  printf ("%sTotal earned in %s:\033[0m %s\n", YELLOW, from_currency,
          formatted_total_from_currency);
  printf ("%sTotal earned in %s:\033[0m %s\n", GREEN, to_currency,
          formatted_total_to_currency);

  return 0;
}
