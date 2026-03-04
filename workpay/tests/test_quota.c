#include "sc_quota.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void
get_today_date (char *output, size_t output_size)
{
  time_t now = time (NULL);
  struct tm local_time;

  assert (now != (time_t)-1);
  assert (localtime_r (&now, &local_time) != NULL);
  assert (strftime (output, output_size, "%Y-%m-%d", &local_time) != 0);
}

static void
create_temp_usage_file_path (char *output, size_t output_size)
{
  int fd;

  assert (output_size >= 32);
  snprintf (output, output_size, "/tmp/workpay_quota_test_XXXXXX");
  fd = mkstemp (output);
  assert (fd != -1);
  close (fd);
  assert (remove (output) == 0);
}

static void
write_usage_file (const char *path, const char *date, int request_count)
{
  FILE *file = fopen (path, "w");
  assert (file != NULL);
  assert (fprintf (file, "%s %d\n", date, request_count) > 0);
  fclose (file);
}

static int
read_usage_count (const char *path, char *date_output, size_t date_output_size)
{
  FILE *file = fopen (path, "r");
  int request_count = -1;

  assert (file != NULL);
  assert (fscanf (file, "%15s %d", date_output, &request_count) == 2);
  fclose (file);

  (void)date_output_size;
  return request_count;
}

static void
test_first_request_creates_usage_file (void)
{
  char path[128];
  char today[16];
  char file_date[16];
  int request_count;

  create_temp_usage_file_path (path, sizeof (path));
  get_today_date (today, sizeof (today));
  assert (setenv ("WORKPAY_USAGE_FILE", path, 1) == 0);

  assert (consume_daily_api_quota () == API_QUOTA_OK);

  request_count = read_usage_count (path, file_date, sizeof (file_date));
  assert (strcmp (file_date, today) == 0);
  assert (request_count == 1);

  assert (unsetenv ("WORKPAY_USAGE_FILE") == 0);
  assert (remove (path) == 0);
}

static void
test_request_99_advances_to_100 (void)
{
  char path[128];
  char today[16];
  char file_date[16];
  int request_count;

  create_temp_usage_file_path (path, sizeof (path));
  get_today_date (today, sizeof (today));
  write_usage_file (path, today, 99);
  assert (setenv ("WORKPAY_USAGE_FILE", path, 1) == 0);

  assert (consume_daily_api_quota () == API_QUOTA_OK);

  request_count = read_usage_count (path, file_date, sizeof (file_date));
  assert (strcmp (file_date, today) == 0);
  assert (request_count == 100);

  assert (unsetenv ("WORKPAY_USAGE_FILE") == 0);
  assert (remove (path) == 0);
}

static void
test_limit_reached_at_100 (void)
{
  char path[128];
  char today[16];
  char file_date[16];
  int request_count;

  create_temp_usage_file_path (path, sizeof (path));
  get_today_date (today, sizeof (today));
  write_usage_file (path, today, 100);
  assert (setenv ("WORKPAY_USAGE_FILE", path, 1) == 0);

  assert (consume_daily_api_quota () == API_QUOTA_REACHED);

  request_count = read_usage_count (path, file_date, sizeof (file_date));
  assert (strcmp (file_date, today) == 0);
  assert (request_count == 100);

  assert (unsetenv ("WORKPAY_USAGE_FILE") == 0);
  assert (remove (path) == 0);
}

int
main (void)
{
  test_first_request_creates_usage_file ();
  test_request_99_advances_to_100 ();
  test_limit_reached_at_100 ();

  printf ("All workpay quota tests passed.\n");
  return 0;
}
