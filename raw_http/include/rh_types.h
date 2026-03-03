#ifndef RH_TYPES_H
#define RH_TYPES_H

#include "rh_config.h"

#include <stddef.h>

typedef struct
{
  char method[RH_METHOD_SIZE];
  char path[RH_PATH_SIZE];
  char version[RH_VERSION_SIZE];
  char host[RH_HOST_SIZE];
  char headers[RH_HEADERS_SIZE];
  char body[RH_BODY_SIZE];
} HttpRequest;

typedef struct
{
  char *headers;
  char *body;
  size_t headers_size;
  size_t body_size;
} HttpResponse;

#endif
