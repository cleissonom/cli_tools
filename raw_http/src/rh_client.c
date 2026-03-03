#include "rh_client.h"

#include "rh_config.h"
#include "rh_headers.h"
#include "rh_response.h"

#include <stdio.h>
#include <string.h>

void
rh_normalize_host (HttpRequest *request)
{
  if (strncmp (request->host, "http://", 7) != 0
      && strncmp (request->host, "https://", 8) != 0)
    {
      char temp_host[RH_HOST_SIZE];
      snprintf (temp_host, sizeof (temp_host), "http://%s", request->host);
      strncpy (request->host, temp_host, sizeof (request->host) - 1);
      request->host[sizeof (request->host) - 1] = '\0';
    }
}

void
rh_build_url (const HttpRequest *request, char *url, size_t url_size)
{
  snprintf (url, url_size, "%s%s", request->host, request->path);
}

static void
rh_apply_method_configuration (CURL *curl, const HttpRequest *request)
{
  if (strcmp (request->method, "POST") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_POST, 1L);
      curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request->body);
    }
  else if (strcmp (request->method, "PUT") == 0
           || strcmp (request->method, "PATCH") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, request->method);
      curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request->body);
    }
  else if (strcmp (request->method, "DELETE") == 0
           || strcmp (request->method, "OPTIONS") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, request->method);
    }
  else if (strcmp (request->method, "HEAD") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_NOBODY, 1L);
    }
}

int
rh_execute_request (const HttpRequest *request, HttpResponse *response,
                    CURLcode *result)
{
  CURL *curl;
  struct curl_slist *headers;
  char url[RH_URL_SIZE];

  curl = curl_easy_init ();
  if (curl == NULL)
    {
      fprintf (stderr, RH_COLOR_ERROR "Error: Could not initialize curl"
                       RH_COLOR_RESET "\n");
      return 0;
    }

  rh_build_url (request, url, sizeof (url));
  curl_easy_setopt (curl, CURLOPT_URL, url);

  headers = rh_configure_http_headers (request);
  curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);

  rh_apply_method_configuration (curl, request);

  curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, rh_header_callback);
  curl_easy_setopt (curl, CURLOPT_HEADERDATA, response);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, rh_write_callback);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, response);

  *result = curl_easy_perform (curl);

  curl_slist_free_all (headers);
  curl_easy_cleanup (curl);

  return 1;
}
