#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <node_api.h>
#include <sqlite3.h>

#include "error.h"
#include "result.h"

static napi_status nsql_result_get_cell(napi_env env, size_t i,
                                        sqlite3_stmt *stmt, napi_value *out);

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

napi_status nsql_result_get_row(napi_env env, sqlite3_stmt *stmt,
                                napi_value *cols, size_t ncols,
                                napi_value *out) {
  napi_escapable_handle_scope scope;
  napi_value result;
  napi_value cell;
  napi_status r2;
  napi_status r;
  size_t i;

  assert(cols != NULL);
  assert(stmt != NULL);
  assert(out != NULL);

  *out = NULL;
  scope = NULL;

  r = napi_open_escapable_handle_scope(env, &scope);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_create_object(env, &result);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  for (i = 0; i < ncols; i++) {
    r = nsql_result_get_cell(env, i, stmt, &cell);

    if (r != napi_ok) {
      goto end;
    }

    r = napi_set_property(env, result, cols[i], cell);

    if (r != napi_ok) {
      nsql_report_error(env, r);

      goto end;
    }
  }

  r = napi_escape_handle(env, scope, result, out);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

end:
  if (scope != NULL) {
    r2 = napi_close_escapable_handle_scope(env, scope);

    if (r2 != napi_ok) {
      nsql_fatal_error(env, r2);
    }
  }

  return r;
}

static napi_status nsql_result_get_cell(napi_env env, size_t i,
                                        sqlite3_stmt *stmt, napi_value *out) {
  napi_status r;
  void *bytes;
  size_t nbytes;
  int type;

  assert(stmt != NULL);
  assert(out != NULL);

  *out = NULL;

  type = sqlite3_column_type(stmt, (int)i);

  switch (type) {
  case SQLITE_NULL:
    r = napi_get_null(env, out);

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    return r;

  case SQLITE_INTEGER:
    r = napi_create_bigint_int64(env, sqlite3_column_int64(stmt, (int)i), out);

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    return r;

  case SQLITE_FLOAT:
    r = napi_create_double(env, sqlite3_column_double(stmt, (int)i), out);

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    return r;

  case SQLITE_TEXT:
    r = napi_create_string_utf8(env,
                                (const char *)sqlite3_column_text(stmt, (int)i),
                                sqlite3_column_bytes(stmt, (int)i), out);

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    return r;

  case SQLITE_BLOB:
    nbytes = sqlite3_column_bytes(stmt, (int)i);

    r = napi_create_arraybuffer(env, nbytes, &bytes, out);

    if (r != napi_ok) {
      nsql_report_error(env, r);

      return r;
    }

    memcpy(bytes, sqlite3_column_blob(stmt, (int)i), nbytes);

    return r;

  default:
    r = napi_throw_error(env, NULL,
                         "Unexpected value type returned in SQLite result set");

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    return r;
  }
}
