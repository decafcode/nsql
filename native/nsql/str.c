#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <node_api.h>

#include "error.h"
#include "str.h"

napi_status nsql_get_string(napi_env env, napi_value value, char **out,
                            size_t *out_nbytes) {
  napi_status r;
  size_t nbytes;
  char *chars;

  assert(out != NULL);

  if (out_nbytes != NULL) {
    *out_nbytes = 0;
  }

  *out = NULL;
  chars = NULL;

  r = napi_get_value_string_utf8(env, value, NULL, 0, &nbytes);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  chars = malloc(nbytes + 1);

  if (chars == NULL) {
    r = nsql_throw_oom(env);

    goto end;
  }

  r = napi_get_value_string_utf8(env, value, chars, nbytes + 1, &nbytes);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (out_nbytes != NULL) {
    *out_nbytes = nbytes;
  }

  *out = chars;
  chars = NULL;

end:
  free(chars);

  return r;
}
