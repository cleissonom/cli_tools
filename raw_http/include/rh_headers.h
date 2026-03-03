#ifndef RH_HEADERS_H
#define RH_HEADERS_H

#include "rh_types.h"

#include <curl/curl.h>

struct curl_slist *rh_configure_http_headers (const HttpRequest *request);

#endif
