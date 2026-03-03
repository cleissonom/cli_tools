#include "rh_client.h"
#include "rh_parse.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
test_parse_get_request (void)
{
  HttpRequest request = { 0 };

  assert (rh_parse_request_file ("tests/fixtures/request_get.txt", &request));
  assert (strcmp (request.method, "GET") == 0);
  assert (strcmp (request.path, "/") == 0);
  assert (strcmp (request.version, "HTTP/1.1") == 0);
  assert (strcmp (request.host, "example.com") == 0);
  assert (strstr (request.headers, "User-Agent: raw_http/1.0") != NULL);
  assert (strstr (request.headers, "Accept: text/html") != NULL);
  assert (strstr (request.headers, "Connection: close") != NULL);
  assert (request.body[0] == '\0');
  assert (rh_validate_required_fields (&request));
}

static void
test_parse_post_request_with_body (void)
{
  HttpRequest request = { 0 };

  assert (rh_parse_request_file ("tests/fixtures/request_post.txt", &request));
  assert (strcmp (request.method, "POST") == 0);
  assert (strcmp (request.path, "/api/items") == 0);
  assert (strcmp (request.version, "HTTP/1.1") == 0);
  assert (strcmp (request.host, "api.example.com") == 0);
  assert (strstr (request.headers, "Content-Type: application/json") != NULL);
  assert (strstr (request.headers, "X-Trace-Id: abc-123") != NULL);
  assert (strstr (request.body, "{\"name\":\"widget\"}") != NULL);
  assert (rh_validate_required_fields (&request));
}

static void
test_normalize_host_and_build_url (void)
{
  HttpRequest request = { 0 };
  char url[RH_URL_SIZE];

  strcpy (request.host, "example.com");
  strcpy (request.path, "/hello");

  rh_normalize_host (&request);
  assert (strcmp (request.host, "http://example.com") == 0);

  rh_build_url (&request, url, sizeof (url));
  assert (strcmp (url, "http://example.com/hello") == 0);
}

int
main (void)
{
  test_parse_get_request ();
  test_parse_post_request_with_body ();
  test_normalize_host_and_build_url ();

  printf ("All raw_http parser tests passed.\n");
  return 0;
}
