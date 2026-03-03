#ifndef RH_CLIENT_H
#define RH_CLIENT_H

#include "rh_types.h"

#include <curl/curl.h>
#include <stddef.h>

void rh_normalize_host (HttpRequest *request);
void rh_build_url (const HttpRequest *request, char *url, size_t url_size);
int rh_execute_request (const HttpRequest *request, HttpResponse *response,
                        CURLcode *result);

#endif
