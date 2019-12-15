#ifndef NDEBUG
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void nsql_dprintf(const char *fmt, ...) {
  va_list ap;

  if (getenv("NSQL_VERBOSE") == NULL) {
    return;
  }

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

#endif
