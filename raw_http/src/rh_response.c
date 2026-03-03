#include "rh_response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
rh_response_init (HttpResponse *response)
{
  response->headers = malloc (1);
  if (response->headers == NULL)
    {
      fprintf (stderr, "Error allocating memory for headers\n");
      return 0;
    }
  response->headers[0] = '\0';
  response->headers_size = 0;

  response->body = malloc (1);
  if (response->body == NULL)
    {
      fprintf (stderr, "Error allocating memory for body\n");
      free (response->headers);
      response->headers = NULL;
      return 0;
    }
  response->body[0] = '\0';
  response->body_size = 0;

  return 1;
}

void
rh_response_free (HttpResponse *response)
{
  free (response->headers);
  free (response->body);
  response->headers = NULL;
  response->body = NULL;
  response->headers_size = 0;
  response->body_size = 0;
}

size_t
rh_header_callback (void *ptr, size_t size, size_t nmemb, void *userdata)
{
  HttpResponse *response = (HttpResponse *)userdata;
  size_t chunk_size = size * nmemb;
  size_t new_length = response->headers_size + chunk_size;
  char *new_headers = realloc (response->headers, new_length + 1);

  if (new_headers == NULL)
    {
      fprintf (stderr, "Error reallocating memory for headers\n");
      return 0;
    }

  response->headers = new_headers;
  memcpy (response->headers + response->headers_size, ptr, chunk_size);
  response->headers_size = new_length;
  response->headers[new_length] = '\0';

  return chunk_size;
}

size_t
rh_write_callback (void *ptr, size_t size, size_t nmemb, void *userdata)
{
  HttpResponse *response = (HttpResponse *)userdata;
  size_t chunk_size = size * nmemb;
  size_t new_length = response->body_size + chunk_size;
  char *new_body = realloc (response->body, new_length + 1);

  if (new_body == NULL)
    {
      fprintf (stderr, "Error reallocating memory for body\n");
      return 0;
    }

  response->body = new_body;
  memcpy (response->body + response->body_size, ptr, chunk_size);
  response->body_size = new_length;
  response->body[new_length] = '\0';

  return chunk_size;
}
