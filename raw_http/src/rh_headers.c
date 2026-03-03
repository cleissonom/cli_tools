#include "rh_headers.h"

#include <string.h>

struct curl_slist *
rh_configure_http_headers (const HttpRequest *request)
{
  struct curl_slist *headers = NULL;
  char header_buffer[RH_HEADERS_SIZE];
  char *header_line;

  strncpy (header_buffer, request->headers, sizeof (header_buffer) - 1);
  header_buffer[sizeof (header_buffer) - 1] = '\0';

  header_line = strtok (header_buffer, "\r\n");
  while (header_line != NULL)
    {
      headers = curl_slist_append (headers, header_line);
      header_line = strtok (NULL, "\r\n");
    }

  return headers;
}
