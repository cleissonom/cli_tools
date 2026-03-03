#include "rh_cli.h"

#include "rh_client.h"
#include "rh_config.h"
#include "rh_output.h"
#include "rh_parse.h"
#include "rh_response.h"
#include "rh_types.h"

#include <curl/curl.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
  const char *input_filename;
  const char *output_filename;
  HttpRequest request = { 0 };
  HttpResponse response = { 0 };
  CURLcode result;

  if (rh_parse_cli_arguments (argc, argv, &input_filename, &output_filename,
                              argv[0])
      != RH_CLI_PARSE_OK)
    {
      return 1;
    }

  if (!rh_parse_request_file (input_filename, &request))
    {
      return 1;
    }

  if (!rh_validate_required_fields (&request))
    {
      return 1;
    }

  rh_normalize_host (&request);

  if (!rh_response_init (&response))
    {
      return 1;
    }

  if (!rh_execute_request (&request, &response, &result))
    {
      rh_response_free (&response);
      return 1;
    }

  if (result != CURLE_OK)
    {
      fprintf (stderr, RH_COLOR_ERROR "Error: %s" RH_COLOR_RESET "\n",
               curl_easy_strerror (result));
    }
  else
    {
      rh_write_output_file (output_filename, &response);
    }

  rh_response_free (&response);

  printf (RH_COLOR_SUCCESS "Request completed successfully!" RH_COLOR_RESET
                           "\n");

  return 0;
}
