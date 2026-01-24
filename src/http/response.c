#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t write_callback(void *ptr, size_t size, size_t nmemb,
                      struct HttpResponse *s) {
  size_t new_len = s->body_size + size * nmemb;
  s->body = realloc(s->body, new_len + 1);
  if (s->body == NULL)
    return 0;
  memcpy(s->body + s->body_size, ptr, size * nmemb);
  s->body[new_len] = '\0';
  s->body_size = new_len;
  return size * nmemb;
}
