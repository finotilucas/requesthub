#include "methods.h"

const char *method_to_string(HttpMethods method) {
  switch (method) {
  case GET:
    return "GET";
  case POST:
    return "POST";
  case PUT:
    return "PUT";
  case DELETE:
    return "DELETE";
  case PATCH:
    return "PATCH";
  case HEAD:
    return "HEAD";
  case OPTIONS:
    return "OPTIONS";
  default:
    return "UNKNOWN";
  }
}
