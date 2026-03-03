#ifndef RH_RESPONSE_H
#define RH_RESPONSE_H

#include "rh_types.h"

#include <stddef.h>

int rh_response_init (HttpResponse *response);
void rh_response_free (HttpResponse *response);
size_t rh_header_callback (void *ptr, size_t size, size_t nmemb,
                           void *userdata);
size_t rh_write_callback (void *ptr, size_t size, size_t nmemb,
                          void *userdata);

#endif
