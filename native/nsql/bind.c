#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "bind.h"
#include "error.h"
#include "str.h"

static napi_status nsql_bind_array(napi_env env, napi_value values,
                                   sqlite3_stmt *stmt, bool *ok);

static napi_status nsql_bind_object(napi_env env, napi_value obj,
                                    sqlite3_stmt *stmt, bool *ok);

static napi_status nsql_bind_get_key(napi_env env, napi_value key,
                                     sqlite3_stmt *stmt, uint32_t *out);

static napi_status nsql_throw_bad_key(napi_env env, napi_value key);

static napi_status nsql_bind_one(napi_env env, napi_value value,
                                 sqlite3_stmt *stmt, uint32_t ordinal,
                                 bool *ok);

static napi_status nsql_bind_null(napi_env env, sqlite3_stmt *stmt,
                                  uint32_t ordinal, bool *ok);

static napi_status nsql_bind_float(napi_env env, napi_value value,
                                   sqlite3_stmt *stmt, uint32_t ordinal,
                                   bool *ok);

static napi_status nsql_bind_string(napi_env env, napi_value value,
                                    sqlite3_stmt *stmt, uint32_t ordinal,
                                    bool *ok);

static napi_status nsql_bind_buffer(napi_env env, napi_value value,
                                    sqlite3_stmt *stmt, uint32_t ordinal,
                                    bool *ok);

static napi_status nsql_bind_bigint(napi_env env, napi_value value,
                                    sqlite3_stmt *stmt, uint32_t ordinal,
                                    bool *ok);

napi_status nsql_bind(napi_env env, napi_value values, sqlite3_stmt *stmt,
                      bool *ok) {
  bool is_array;
  napi_valuetype type;
  napi_status r;
  int sqlr;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;

  r = napi_typeof(env, values, &type);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (type != napi_object) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "Bind parameters must be an array or object");

    goto end;
  }

  r = napi_is_array(env, values, &is_array);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (is_array) {
    r = nsql_bind_array(env, values, stmt, ok);
  } else {
    r = nsql_bind_object(env, values, stmt, ok);
  }

  if (r != napi_ok || !*ok) {
    sqlr = sqlite3_clear_bindings(stmt);

    if (sqlr != SQLITE_OK) {
      nsql_fatal_sqlite_error(sqlr);
    }
  }

end:
  return r;
}

static napi_status nsql_bind_array(napi_env env, napi_value values,
                                   sqlite3_stmt *stmt, bool *ok) {
  napi_value value;
  napi_status r;
  uint32_t len;
  uint32_t i;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;

  r = napi_get_array_length(env, values, &len);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  for (i = 0; i < len; i++) {
    r = napi_get_element(env, values, i, &value);

    if (r != napi_ok) {
      nsql_report_error(env, r);

      goto end;
    }

    r = nsql_bind_one(env, value, stmt, i + 1, ok);

    if (r != napi_ok || !*ok) {
      goto end;
    }
  }

  *ok = true;

end:
  return r;
}

static napi_status nsql_bind_object(napi_env env, napi_value obj,
                                    sqlite3_stmt *stmt, bool *ok) {
  napi_value props;
  napi_value key;
  napi_value value;
  uint32_t ordinal;
  uint32_t nprops;
  uint32_t i;
  napi_status r;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;

  r = napi_get_property_names(env, obj, &props);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_get_array_length(env, props, &nprops);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  for (i = 0; i < nprops; i++) {
    r = napi_get_element(env, props, i, &key);

    if (r != napi_ok) {
      nsql_report_error(env, r);

      goto end;
    }

    r = nsql_bind_get_key(env, key, stmt, &ordinal);

    if (r != napi_ok || ordinal == 0) {
      goto end;
    }

    r = napi_get_property(env, obj, key, &value);

    if (r != napi_ok) {
      nsql_report_error(env, r);

      goto end;
    }

    r = nsql_bind_one(env, value, stmt, ordinal, ok);

    if (r != napi_ok || !*ok) {
      goto end;
    }
  }

  *ok = true;

end:
  return r;
}

static napi_status nsql_bind_get_key(napi_env env, napi_value key,
                                     sqlite3_stmt *stmt, uint32_t *out) {
  napi_status r;
  uint32_t ordinal;
  char *ckey;

  assert(stmt != NULL);
  assert(out != NULL);

  *out = 0;
  ckey = NULL;

  r = nsql_get_string(env, key, &ckey, NULL);

  if (r != napi_ok || ckey == NULL) {
    goto end;
  }

  ordinal = sqlite3_bind_parameter_index(stmt, ckey);

  if (ordinal == 0) {
    r = nsql_throw_bad_key(env, key);

    goto end;
  }

  *out = ordinal;

end:
  free(ckey);

  return r;
}

static napi_status nsql_throw_bad_key(napi_env env, napi_value key) {
  napi_value msg;
  napi_value error;
  napi_status r;

  r = napi_create_string_utf8(
      env, "A named bind parameter is not present in the query",
      NAPI_AUTO_LENGTH, &msg);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_create_error(env, NULL, msg, &error);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_set_named_property(env, error, "name", key);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_throw(env, error);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

end:
  return r;
}

static napi_status nsql_bind_one(napi_env env, napi_value value,
                                 sqlite3_stmt *stmt, uint32_t ordinal,
                                 bool *ok) {
  napi_valuetype type;
  napi_status r;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;

  r = napi_typeof(env, value, &type);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    return r;
  }

  switch (type) {
  case napi_null:
    return nsql_bind_null(env, stmt, ordinal, ok);

  case napi_number:
    return nsql_bind_float(env, value, stmt, ordinal, ok);

  case napi_string:
    return nsql_bind_string(env, value, stmt, ordinal, ok);

  case napi_object:
    return nsql_bind_buffer(env, value, stmt, ordinal, ok);

  case napi_bigint:
    return nsql_bind_bigint(env, value, stmt, ordinal, ok);

  default:
    return napi_throw_type_error(
        env, "ERR_INVALID_ARG_TYPE",
        "Unsupported parameter type passed to prepared statement");
  }
}

static napi_status nsql_bind_null(napi_env env, sqlite3_stmt *stmt,
                                  uint32_t ordinal, bool *ok) {
  int sqlr;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;
  sqlr = sqlite3_bind_null(stmt, ordinal);

  if (sqlr != SQLITE_OK) {
    return nsql_throw_sqlite_error(env, sqlr, NULL);
  }

  *ok = true;

  return napi_ok;
}

static napi_status nsql_bind_float(napi_env env, napi_value value,
                                   sqlite3_stmt *stmt, uint32_t ordinal,
                                   bool *ok) {
  double num;
  napi_status r;
  int sqlr;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;
  r = napi_get_value_double(env, value, &num);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  sqlr = sqlite3_bind_double(stmt, ordinal, num);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  *ok = true;

end:
  return r;
}

static napi_status nsql_bind_string(napi_env env, napi_value value,
                                    sqlite3_stmt *stmt, uint32_t ordinal,
                                    bool *ok) {
  char *str;
  size_t nbytes;
  napi_status r;
  int sqlr;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;
  str = NULL;

  r = nsql_get_string(env, value, &str, &nbytes);

  if (r != napi_ok || str == NULL) {
    goto end;
  }

  /* This function is unusual: whether it succeeds or fails it takes immediate
     ownership of `str` regardless. */

  sqlr = sqlite3_bind_text(stmt, ordinal, str, (int)nbytes, free);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  *ok = true;

end:
  return r;
}

static napi_status nsql_bind_buffer(napi_env env, napi_value value,
                                    sqlite3_stmt *stmt, uint32_t ordinal,
                                    bool *ok) {
  bool is_buffer;
  void *bytes;
  size_t nbytes;
  napi_status r;
  int sqlr;

  assert(stmt != NULL);
  assert(ok != NULL);

  *ok = false;

  r = napi_is_arraybuffer(env, value, &is_buffer);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (!is_buffer) {
    r = napi_throw_type_error(
        env, "ERR_INVALID_ARG_TYPE",
        "Object parameter to prepared statement is not an ArrayBuffer");

    goto end;
  }

  r = napi_get_arraybuffer_info(env, value, &bytes, &nbytes);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (nbytes > INT_MAX) {
    /* like this is ever going to get executed */
    r = napi_throw_type_error(env, NULL,
                              "ArrayBuffer size exceeds SQLite limits");

    goto end;
  }

  sqlr = sqlite3_bind_blob(stmt, ordinal, bytes, (int)nbytes, SQLITE_TRANSIENT);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  *ok = true;

end:
  return r;
}

static napi_status nsql_bind_bigint(napi_env env, napi_value value,
                                    sqlite3_stmt *stmt, uint32_t ordinal,
                                    bool *ok) {
  bool fit;
  int64_t num;
  napi_status r;
  int sqlr;

  assert(stmt != NULL);
  assert(ok != NULL);

  r = napi_get_value_bigint_int64(env, value, &num, &fit);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (!fit) {
    r = napi_throw_range_error(
        env, "ERR_VALUE_OUT_OF_RANGE",
        "Bigint bind parameter does not fit in a 64-bit int");

    goto end;
  }

  sqlr = sqlite3_bind_int64(stmt, ordinal, num);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  *ok = true;

end:
  return r;
}
