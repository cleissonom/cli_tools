#include "rh_parse.h"

#include "rh_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

void
rh_parse_line (char *line, HttpRequest *request, int *is_body)
{
  if (strcmp (line, "\r\n") == 0 || strcmp (line, "\n") == 0)
    {
      *is_body = 1;
      return;
    }

  if (!(*is_body))
    {
      if (strstr (line, "GET") == line || strstr (line, "POST") == line
          || strstr (line, "PUT") == line || strstr (line, "DELETE") == line
          || strstr (line, "PATCH") == line || strstr (line, "OPTIONS") == line
          || strstr (line, "HEAD") == line)
        {
          sscanf (line, "%9s %1023s %9s", request->method, request->path,
                  request->version);
        }
      else if (strstr (line, "Host:") == line)
        {
          sscanf (line, "Host: %1023s", request->host);
        }
      else
        {
          strncat (request->headers, line,
                   sizeof (request->headers) - strlen (request->headers) - 1);
          strncat (request->headers, "\r\n",
                   sizeof (request->headers) - strlen (request->headers) - 1);
        }
    }
  else
    {
      strncat (request->body, line,
               sizeof (request->body) - strlen (request->body) - 1);
    }
}

int
rh_parse_request_file (const char *input_filename, HttpRequest *request)
{
  FILE *input_file;
  char *line = NULL;
  size_t line_capacity = 0;
  ssize_t line_length;
  int is_body = 0;

  input_file = fopen (input_filename, "r");
  if (input_file == NULL)
    {
      fprintf (stderr, RH_COLOR_ERROR "Error: Could not open file %s"
                       RH_COLOR_RESET "\n",
               input_filename);
      return 0;
    }

  while ((line_length = getline (&line, &line_capacity, input_file)) != -1)
    {
      (void)line_length;
      rh_parse_line (line, request, &is_body);
    }

  free (line);
  fclose (input_file);

  return 1;
}

int
rh_validate_required_fields (const HttpRequest *request)
{
  if (strlen (request->method) == 0 || strlen (request->path) == 0
      || strlen (request->version) == 0 || strlen (request->host) == 0)
    {
      fprintf (stderr,
               RH_COLOR_ERROR "Error: Missing required fields (Method, Path, "
                              "HTTP version, or Host)"
                              RH_COLOR_RESET "\n");
      return 0;
    }

  return 1;
}
