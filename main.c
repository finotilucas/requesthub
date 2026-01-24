#include "src/http/methods.h"
#include "src/http/request.h"
#include "src/http/response.h"

#include <stdio.h>
#include <time.h>

int main() {
  clock_t start = clock();

  HttpRequest *request = NULL;
  const char *url = "https://jsonplaceholder.typicode.com/todos/1";

  request = http_request_new(url, GET);

  if (request == NULL) {
    return 1;
  }

  http_request_perform(request);

  http_request_free(request);
  request = NULL;

  clock_t stop = clock();
  double elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
  printf("Time elapsed in ms: %.2f\n", elapsed);

  return 0;
}
