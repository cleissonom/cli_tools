#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *BLUE = "\033[1;34m";
const char *YELLOW = "\033[1;33m";
const char *GREEN = "\033[1;32m";

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
  mem->memory[mem->size] = 0;

  return realsize;
}

void
print_usage (FILE *stream, const char *program_name)
{
  fprintf (stream, "Usage: %s [--help] <hourly_rate> <hours:minutes:seconds>\n",
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
  printf ("\n");
  printf ("Examples:\n");
  printf ("  %s 25 08:30:45\n", program_name);
}

double
get_exchange_rate (void)
{
  CURL *curl_handle;
  CURLcode res;
  struct MemoryStruct chunk;

  chunk.memory = malloc (1);
  if (chunk.memory == NULL)
    {
      fprintf (stderr, "Not enough memory to allocate\n");
      return 0.0;
    }
  chunk.size = 0;

  if (curl_global_init (CURL_GLOBAL_ALL) != CURLE_OK)
    {
      fprintf (stderr, "curl_global_init() failed\n");
      free (chunk.memory);
      return 0.0;
    }

  curl_handle = curl_easy_init ();
  if (curl_handle == NULL)
    {
      fprintf (stderr, "curl_easy_init() failed\n");
      free (chunk.memory);
      curl_global_cleanup ();
      return 0.0;
    }

  curl_easy_setopt (curl_handle, CURLOPT_URL,
                    "https://economia.awesomeapi.com.br/json/last/USD-BRL");
  curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt (curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  res = curl_easy_perform (curl_handle);
  double exchange_rate = 0.0;

  if (res != CURLE_OK)
    {
      fprintf (stderr, "curl_easy_perform() failed: %s\n",
               curl_easy_strerror (res));
    }
  else
    {
      char *start = strstr (chunk.memory, "\"bid\":\"");
      if (start)
        {
          start += 7;
          exchange_rate = strtod (start, NULL);
        }
      else
        {
          fprintf (stderr, "Failed to find exchange rate in response\n");
        }
    }

  curl_easy_cleanup (curl_handle);
  free (chunk.memory);
  curl_global_cleanup ();

  return exchange_rate;
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
main (int argc, char *argv[])
{
  if (argc == 2
      && (strcmp (argv[1], "--help") == 0 || strcmp (argv[1], "-h") == 0))
    {
      print_help (argv[0]);
      return 0;
    }

  if (argc != 3)
    {
      fprintf (stderr, "\033[1;31m");
      print_usage (stderr, argv[0]);
      fprintf (stderr, "\033[0m");
      return 1;
    }

  double hourly_rate;
  int hours, minutes, seconds;

  if (!parse_hourly_rate (argv[1], &hourly_rate))
    {
      fprintf (stderr,
               "\033[1;31mInvalid hourly rate. Use a non-negative number.\033"
               "[0m\n");
      return 1;
    }

  if (!parse_time (argv[2], &hours, &minutes, &seconds))
    {
      fprintf (
          stderr,
          "\033[1;31mInvalid time format. Use HH:MM:SS with MM/SS between 00 "
          "and 59.\033[0m\n");
      return 1;
    }

  double total_earned_usd
      = calculate_payment (hours, minutes, seconds, hourly_rate);
  double exchange_rate = get_exchange_rate ();
  if (exchange_rate == 0.0)
    {
      fprintf (stderr, "Failed to retrieve exchange rate\n");
      return 1;
    }
  double total_earned_brl = total_earned_usd * exchange_rate;

  printf ("%sTotal hours worked:\033[0m %.2f hours\n", BLUE,
          (double)hours + (minutes / 60.0) + (seconds / 3600.0));
  printf ("%sHourly wage:\033[0m $%.2f\n", BLUE, hourly_rate);
  printf ("%sDollar exchange rate:\033[0m R$%.2f\n", YELLOW, exchange_rate);
  printf ("%sTotal earned in dollars:\033[0m $%.2f\n", YELLOW,
          total_earned_usd);
  printf ("%sTotal earned in reais:\033[0m R$%.2f\n", GREEN, total_earned_brl);

  return 0;
}
