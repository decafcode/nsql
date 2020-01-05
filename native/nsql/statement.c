#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "bind.h"
#include "dprintf.h"
#include "error.h"
#include "macros.h"
#include "result.h"
#include "statement.h"
#include "str.h"

struct nsql_statement {
  /* SQLite connection object ownership is handled through internal reference
     counting within SQLite itself. Calling `sqlite3_close_v2()` to dispose of
     a database connection does not actually cause the connection to be cleaned
     up and deallocated until the last statement using that connection has been
     destroyed as well.

     We need to hold on to the originating database connection object because
     we need access to that object in order to retrieve error messages and last
     rowid values. */

  sqlite3 *db;
  sqlite3_stmt *stmt;
};

static napi_value nsql_statement_constructor(napi_env env,
                                             napi_callback_info ctx);

static void nsql_statement_destructor(napi_env env, void *ptr, void *hint);

static napi_value nsql_statement_close(napi_env env, napi_callback_info ctx);

static void nsql_statement_reset(struct nsql_statement *self);

static napi_status nsql_statement_exec_preamble(napi_env env,
                                                napi_callback_info ctx,
                                                struct nsql_statement **out);

static napi_value nsql_statement_run(napi_env env, napi_callback_info ctx);

static napi_status nsql_statement_run_result(napi_env env, sqlite3 *db,
                                             napi_value *out);

static napi_value nsql_statement_one(napi_env env, napi_callback_info ctx);

static napi_value nsql_statement_all(napi_env env, napi_callback_info ctx);

static napi_value nsql_statement_get_sql(napi_env env, napi_callback_info ctx);

static const napi_property_descriptor nsql_statement_desc[] = {
    {.utf8name = "close", .method = nsql_statement_close},
    {.utf8name = "run", .method = nsql_statement_run},
    {.utf8name = "one", .method = nsql_statement_one},
    {.utf8name = "all", .method = nsql_statement_all},
    {.utf8name = "sql", .getter = nsql_statement_get_sql}};

napi_status nsql_statement_define_class(napi_env env, napi_value *out) {
  napi_value nclass;
  napi_status r;

  assert(out != NULL);

  *out = NULL;

  nsql_dprintf("%s\n", __func__);

  r = napi_define_class(
      env, "Statement", NAPI_AUTO_LENGTH, nsql_statement_constructor, NULL,
      countof(nsql_statement_desc), nsql_statement_desc, &nclass);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  *out = nclass;

end:
  return r;
}

napi_status nsql_statement_prepare(napi_env env, napi_value nclass, sqlite3 *db,
                                   napi_value nsql, napi_value *out) {
  struct nsql_statement *self;
  char *sql;
  const char *sql_end;
  size_t sql_nbytes;
  napi_valuetype type;
  napi_value nself;
  napi_status r;
  int sqlr;

  assert(db != NULL);
  assert(out != NULL);

  *out = NULL;
  sql = NULL;

  r = napi_typeof(env, nsql, &type);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (type != napi_string) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "sql: Expected string");

    goto end;
  }

  r = napi_new_instance(env, nclass, 0, NULL, &nself);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_unwrap(env, nself, (void **)&self);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = nsql_get_string(env, nsql, &sql, &sql_nbytes);

  if (r != napi_ok || sql == NULL) {
    goto end;
  }

  sqlr = sqlite3_prepare_v2(db, sql, -1, &self->stmt, &sql_end);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, db);

    goto end;
  }

  if (sql_end != sql + sql_nbytes) {
    r = napi_throw_error(env, "ERR_INVALID_ARG_VALUE",
                         "Trailing characters in SQL statement");

    goto end;
  }

  self->db = db;
  *out = nself;

end:
  free(sql);

  return r;
}

static napi_value nsql_statement_constructor(napi_env env,
                                             napi_callback_info ctx) {
  struct nsql_statement *self;
  napi_value nself;
  napi_value out;
  napi_status r;

  out = NULL;
  self = calloc(1, sizeof(*self));

  if (self == NULL) {
    r = nsql_throw_oom(env);

    goto end;
  }

  r = napi_get_cb_info(env, ctx, NULL, NULL, &nself, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_wrap(env, nself, self, nsql_statement_destructor, NULL, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  nsql_dprintf("%s -> %p\n", __func__, self);

  out = nself;
  self = NULL;

end:
  nsql_statement_destructor(env, self, NULL);

  return nsql_return(env, r, out);
}

static void nsql_statement_destructor(napi_env env, void *ptr, void *hint) {
  struct nsql_statement *self;
  int sqlr;

  if (ptr == NULL) {
    return;
  }

  nsql_dprintf("%s(%p)\n", __func__, ptr);

  self = ptr;
  sqlr = sqlite3_finalize(self->stmt);

  if (sqlr != SQLITE_OK) {
    nsql_fatal_sqlite_error(sqlr);
  }

  free(self);
}

static napi_value nsql_statement_close(napi_env env, napi_callback_info ctx) {
  struct nsql_statement *self;
  napi_value nself;
  napi_status r;
  int sqlr;

  r = napi_get_cb_info(env, ctx, NULL, NULL, &nself, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_unwrap(env, nself, (void **)&self);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  assert(self != NULL);

  sqlr = sqlite3_finalize(self->stmt);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);
  }

  self->db = NULL;
  self->stmt = NULL;

  nsql_dprintf("%s\n", __func__);

end:
  return nsql_return(env, r, NULL);
}

static void nsql_statement_reset(struct nsql_statement *self) {
  int sqlr;

  if (self == NULL || self->stmt == NULL) {
    return;
  }

  (void)sqlite3_reset(self->stmt);

  /* sqlite3_reset() returns the last error encountered by this statement's
     latest execution, not the success or failure of the reset operation
     itself. */

  sqlr = sqlite3_clear_bindings(self->stmt);

  if (sqlr != SQLITE_OK) {
    nsql_fatal_sqlite_error(sqlr);
  }
}

static napi_status nsql_statement_exec_preamble(napi_env env,
                                                napi_callback_info ctx,
                                                struct nsql_statement **out) {
  struct nsql_statement *self;
  size_t argc;
  napi_value argv[1];
  napi_value nself;
  napi_status r;
  bool ok;

  assert(out != NULL);

  *out = NULL;
  self = NULL;

  argc = countof(argv);
  r = napi_get_cb_info(env, ctx, &argc, argv, &nself, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_unwrap(env, nself, (void **)&self);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  assert(self != NULL);

  if (self->stmt == NULL) {
    r = napi_throw_error(env, NULL, "Attempted to execute a closed statement");

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    goto end;
  }

  if (argc > 0) {
    r = nsql_bind(env, argv[0], self->stmt, &ok);

    if (r != napi_ok || !ok) {
      nsql_statement_reset(self);

      goto end;
    }
  }

  *out = self;

end:
  return r;
}

static napi_value nsql_statement_run(napi_env env, napi_callback_info ctx) {
  struct nsql_statement *self;
  napi_value result;
  napi_status r;
  int sqlr;

  self = NULL;
  result = NULL;

  r = nsql_statement_exec_preamble(env, ctx, &self);

  if (r != napi_ok || self == NULL) {
    goto end;
  }

  do {
    sqlr = sqlite3_step(self->stmt);
  } while (sqlr == SQLITE_ROW);

  if (sqlr != SQLITE_DONE) {
    r = nsql_throw_sqlite_error(env, sqlr, self->db);

    goto end;
  }

  r = nsql_statement_run_result(env, self->db, &result);

end:
  nsql_statement_reset(self);

  return nsql_return(env, r, result);
}

static napi_status nsql_statement_run_result(napi_env env, sqlite3 *db,
                                             napi_value *out) {
  napi_value changes;
  napi_value rowid;
  napi_value obj;
  napi_status r;

  assert(db != NULL);
  assert(out != NULL);

  *out = NULL;

  r = napi_create_object(env, &obj);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_create_int32(env, sqlite3_changes(db), &changes);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_create_bigint_int64(env, sqlite3_last_insert_rowid(db), &rowid);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_set_named_property(env, obj, "changes", changes);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_set_named_property(env, obj, "lastInsertRowid", rowid);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  *out = obj;

end:
  return r;
}

static napi_value nsql_statement_one(napi_env env, napi_callback_info ctx) {
  struct nsql_statement *self;
  size_t ncols;
  napi_value *cols;
  napi_value result;
  napi_status r;
  int sqlr;

  self = NULL;
  cols = NULL;
  result = NULL;

  r = nsql_statement_exec_preamble(env, ctx, &self);

  if (r != napi_ok || self == NULL) {
    goto end;
  }

  sqlr = sqlite3_step(self->stmt);

  switch (sqlr) {
  case SQLITE_DONE:
    r = napi_get_undefined(env, &result);

    if (r != napi_ok) {
      nsql_report_error(env, r);
    }

    break;

  case SQLITE_ROW:
    r = nsql_result_get_columns(env, self->stmt, &cols, &ncols);

    if (r != napi_ok) {
      goto end;
    }

    r = nsql_result_get_row(env, self->stmt, cols, ncols, &result);

    break;

  default:
    r = nsql_throw_sqlite_error(env, sqlr, self->db);

    break;
  }

end:
  nsql_statement_reset(self);
  free(cols);

  return nsql_return(env, r, result);
}

static napi_value nsql_statement_all(napi_env env, napi_callback_info ctx) {
  struct nsql_statement *self;
  size_t ncols;
  napi_value *cols;
  napi_value result;
  napi_value out;
  napi_status r;
  int sqlr;

  out = NULL;
  cols = NULL;

  r = nsql_statement_exec_preamble(env, ctx, &self);

  if (r != napi_ok || self == NULL) {
    goto end;
  }

  r = napi_create_array(env, &result);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  for (;;) {
    sqlr = sqlite3_step(self->stmt);

    if (sqlr == SQLITE_DONE) {
      break;
    }

    if (sqlr != SQLITE_ROW) {
      r = nsql_throw_sqlite_error(env, sqlr, self->db);

      goto end;
    }

    /* We need to JavaScriptify the column names before we can process the
       first row. */

    if (cols == NULL) {
      r = nsql_result_get_columns(env, self->stmt, &cols, &ncols);

      if (r != napi_ok || cols == NULL) {
        goto end;
      }
    }

    r = nsql_result_push_row(env, self->stmt, cols, ncols, result);

    if (r != napi_ok) {
      goto end;
    }
  }

  out = result;

end:
  nsql_statement_reset(self);
  free(cols);

  return nsql_return(env, r, out);
}

static napi_value nsql_statement_get_sql(napi_env env, napi_callback_info ctx) {
  const char *str;
  struct nsql_statement *self;
  napi_value nself;
  napi_value out;
  napi_status r;

  out = NULL;

  r = napi_get_cb_info(env, ctx, NULL, NULL, &nself, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_unwrap(env, nself, (void **)&self);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  assert(self != NULL);

  if (self->stmt != NULL) {
    str = sqlite3_sql(self->stmt);

    if (str == NULL) {
      r = nsql_throw_oom(env);

      goto end;
    }
  } else {
    str = "#CLOSED";
  }

  r = napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &out);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

end:
  return nsql_return(env, r, out);
}
