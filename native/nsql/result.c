#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "error.h"
#include "result.h"

napi_status nsql_result_get_columns(napi_env env, sqlite3_stmt *stmt,
                                    napi_value **out_cols, size_t *out_ncols) {
  const char *col;
  napi_value *cols;
  napi_status r;
  size_t ncols;
  size_t i;

  assert(stmt != NULL);
  assert(out_cols != NULL);

  cols = NULL;
  *out_cols = NULL;
  *out_ncols = 0;

  /* Does any real implementation of malloc et al return NULL for malloc(0)?
     Spec says it is permitted to. Allocate a one-element array for the zero-
     column case anyway just to be safe. */

  ncols = sqlite3_column_count(stmt);

  if (ncols > 0) {
    cols = calloc(ncols, sizeof(napi_value));
  } else {
    cols = calloc(1, sizeof(napi_value));
  }

  if (cols == NULL) {
    r = nsql_throw_oom(env);

    goto end;
  }

  for (i = 0; i < ncols; i++) {
    col = sqlite3_column_name(stmt, (int)i);

    r = napi_create_string_utf8(env, col, NAPI_AUTO_LENGTH, &cols[i]);

    if (r != napi_ok) {
      nsql_report_error(env, r);

      goto end;
    }
  }

  *out_cols = cols;
  *out_ncols = ncols;
  cols = NULL;
  r = napi_ok;

end:
  free(cols);

  return r;
}
