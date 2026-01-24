#ifndef METHODS_H
#define METHODS_H

typedef enum {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  HEAD,
  OPTIONS,
} HttpMethods;

const char *method_to_string(HttpMethods method);

#endif
