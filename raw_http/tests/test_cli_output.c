#include "rh_cli.h"
#include "rh_output.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
test_cli_parse_with_required_input_only (void)
{
  const char *input_filename = NULL;
  const char *output_filename = "placeholder";
  char *argv[] = { "raw_http", "request.txt" };

  assert (rh_parse_cli_arguments (2, argv, &input_filename, &output_filename,
                                  argv[0])
          == RH_CLI_PARSE_OK);
  assert (strcmp (input_filename, "request.txt") == 0);
  assert (output_filename == NULL);
}

static void
test_cli_parse_with_output_file (void)
{
  const char *input_filename = NULL;
  const char *output_filename = NULL;
  char *argv[] = { "raw_http", "request.txt", "response.txt" };

  assert (rh_parse_cli_arguments (3, argv, &input_filename, &output_filename,
                                  argv[0])
          == RH_CLI_PARSE_OK);
  assert (strcmp (input_filename, "request.txt") == 0);
  assert (strcmp (output_filename, "response.txt") == 0);
}

static void
test_cli_parse_invalid_arity (void)
{
  const char *input_filename = NULL;
  const char *output_filename = NULL;
  char *argv0[] = { "raw_http" };
  char *argv3[] = { "raw_http", "a", "b", "c" };

  assert (rh_parse_cli_arguments (1, argv0, &input_filename, &output_filename,
                                  argv0[0])
          == RH_CLI_PARSE_ERROR);
  assert (rh_parse_cli_arguments (4, argv3, &input_filename, &output_filename,
                                  argv3[0])
          == RH_CLI_PARSE_ERROR);
}

static void
test_write_output_stream (void)
{
  HttpResponse response = { 0 };
  FILE *stream;
  char buffer[256];
  size_t read_size;

  response.headers = "Header: value";
  response.body = "body content";

  stream = tmpfile ();
  assert (stream != NULL);

  rh_write_output_stream (stream, &response);
  fflush (stream);
  rewind (stream);

  read_size = fread (buffer, 1, sizeof (buffer) - 1, stream);
  buffer[read_size] = '\0';

  assert (strcmp (buffer, "Header: value\nbody content") == 0);

  fclose (stream);
}

int
main (void)
{
  test_cli_parse_with_required_input_only ();
  test_cli_parse_with_output_file ();
  test_cli_parse_invalid_arity ();
  test_write_output_stream ();

  printf ("All raw_http CLI/output tests passed.\n");
  return 0;
}
