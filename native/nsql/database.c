#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "dprintf.h"
#include "error.h"
#include "macros.h"
#include "statement.h"
#include "str.h"

struct nsql_database_class {
  napi_ref stmt_class;
};

struct nsql_database {
  struct nsql_database_class *class_;
  sqlite3 *db;
};

static void nsql_database_class_destructor(napi_env env, void *ptr, void *hint);

static napi_value nsql_database_constructor(napi_env env,
                                            napi_callback_info ctx);

static void nsql_database_destructor(napi_env env, void *ptr, void *hint);

static napi_value nsql_database_close(napi_env env, napi_callback_info ctx);

static napi_value nsql_database_exec(napi_env env, napi_callback_info ctx);

static napi_value nsql_database_prepare(napi_env env, napi_callback_info ctx);

static napi_value nsql_database_get_db_filename(napi_env env,
                                                napi_callback_info ctx);

static const napi_property_descriptor nsql_database_desc[] = {
    {.utf8name = "close", .method = nsql_database_close},
    {.utf8name = "exec", .method = nsql_database_exec},
    {.utf8name = "prepare", .method = nsql_database_prepare},
    {.utf8name = "dbFilename", .getter = nsql_database_get_db_filename}};

napi_status nsql_database_define_class(napi_env env, napi_value *out) {
  struct nsql_database_class *class_;
  napi_value stmt_nclass;
  napi_value nclass;
  napi_status r;

  assert(out != NULL);

  class_ = NULL;

  nsql_dprintf("%s\n", __func__);

  class_ = calloc(1, sizeof(*class_));

  if (class_ == NULL) {
    r = nsql_throw_oom(env);

    goto end;
  }

  r = nsql_statement_define_class(env, &stmt_nclass);

  if (r != napi_ok) {
    goto end;
  }

  r = napi_create_reference(env, stmt_nclass, 1, &class_->stmt_class);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_define_class(
      env, "Database", NAPI_AUTO_LENGTH, nsql_database_constructor, class_,
      countof(nsql_database_desc), nsql_database_desc, &nclass);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  /* Expose the Statement constructor to JavaScript so that we can add a REPL
     inspector for it. JS code shouldn't be instantiating it directly though;
     hopefully the leading underscore will get the point across. */

  r = napi_set_named_property(env, nclass, "_Statement", stmt_nclass);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  r = napi_add_finalizer(env, nclass, class_, nsql_database_class_destructor,
                         NULL, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  class_ = NULL;
  *out = nclass;

end:
  nsql_database_class_destructor(env, class_, NULL);

  return r;
}

static void nsql_database_class_destructor(napi_env env, void *ptr,
                                           void *hint) {
  struct nsql_database_class *class_;
  napi_status r;

  if (ptr == NULL) {
    return;
  }

  nsql_dprintf("%s(%p)\n", __func__, ptr);

  class_ = ptr;

  if (class_->stmt_class != NULL) {
    r = napi_delete_reference(env, class_->stmt_class);

    if (r != napi_ok) {
      nsql_fatal_error(env, r);
    }
  }

  free(class_);
}

static napi_value nsql_database_constructor(napi_env env,
                                            napi_callback_info ctx) {
  char *uri;
  struct nsql_database_class *class_;
  struct nsql_database *self;
  size_t argc;
  napi_valuetype type;
  napi_value argv[1];
  napi_value target;
  napi_value nself;
  napi_status r;
  int sqlr;

  uri = NULL;
  self = NULL;
  nself = NULL;

  /* Check constructor invocation */

  r = napi_get_new_target(env, ctx, &target);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (target == NULL) {
    r = napi_throw_type_error(env, NULL, "Constructor Database requires 'new'");

    goto end;
  }

  /* Collect and validate params */

  argc = countof(argv);
  r = napi_get_cb_info(env, ctx, &argc, argv, &nself, (void **)&class_);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (argc < 1) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "Expected a URI parameter");

    goto end;
  }

  r = napi_typeof(env, argv[0], &type);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (type != napi_string) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "uri: Expected string");

    goto end;
  }

  r = nsql_get_string(env, argv[0], &uri, NULL);

  if (r != napi_ok || uri == NULL) {
    goto end;
  }

  /* Construct wrapper object */

  self = calloc(1, sizeof(*self));

  if (self == NULL) {
    r = nsql_throw_oom(env);

    goto end;
  }

  self->class_ = class_;
  sqlr = sqlite3_open_v2(uri, &self->db,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  sqlr = sqlite3_extended_result_codes(self->db, 1);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  /* Bind wrapper object */

  r = napi_wrap(env, nself, self, nsql_database_destructor, NULL, NULL);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  nsql_dprintf("%s(\"%s\") -> %p\n", __func__, uri, self);
  self = NULL;

end:
  free(uri);
  nsql_database_destructor(env, self, NULL);

  return nsql_return(env, r, nself);
}

static void nsql_database_destructor(napi_env env, void *ptr, void *hint) {
  struct nsql_database *self;
  int sqlr;

  if (ptr == NULL) {
    return;
  }

  nsql_dprintf("%s(%p)\n", __func__, ptr);

  self = ptr;
  sqlr = sqlite3_close_v2(self->db);

  if (sqlr != SQLITE_OK) {
    nsql_fatal_sqlite_error(sqlr);
  }

  free(self);
}

static napi_value nsql_database_close(napi_env env, napi_callback_info ctx) {
  struct nsql_database *self;
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

  sqlr = sqlite3_close_v2(self->db);

  if (sqlr != SQLITE_OK) {
    nsql_throw_sqlite_error(env, sqlr, NULL);

    goto end;
  }

  self->db = NULL;
  nsql_dprintf("%s\n", __func__);

end:
  return nsql_return(env, r, NULL);
}

static napi_value nsql_database_exec(napi_env env, napi_callback_info ctx) {
  struct nsql_database *self;
  char *sql;
  size_t argc;
  napi_valuetype type;
  napi_value argv[1];
  napi_value nself;
  napi_status r;
  int sqlr;

  sql = NULL;

  /* Get `this` */

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

  /* Validate and unpack params */

  if (argc < 1) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "Expected an SQL parameter");

    goto end;
  }

  r = napi_typeof(env, argv[0], &type);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (type != napi_string) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "sql: Expected string");

    goto end;
  }

  r = nsql_get_string(env, argv[0], &sql, NULL);

  if (r != napi_ok || sql == NULL) {
    goto end;
  }

  /* Call through to SQLite */

  sqlr = sqlite3_exec(self->db, sql, NULL, NULL, NULL);

  if (sqlr != SQLITE_OK) {
    r = nsql_throw_sqlite_error(env, sqlr, self->db);
  }

end:
  free(sql);

  return nsql_return(env, r, NULL);
}

static napi_value nsql_database_prepare(napi_env env, napi_callback_info ctx) {
  struct nsql_database *self;
  size_t argc;
  napi_value argv[1];
  napi_value nclass_stmt;
  napi_value nself;
  napi_value out;
  napi_status r;

  out = NULL;

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
  assert(self->class_ != NULL);

  r = napi_get_reference_value(env, self->class_->stmt_class, &nclass_stmt);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

  if (argc < 1) {
    r = napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE",
                              "Expected an SQL parameter");

    goto end;
  }

  r = nsql_statement_prepare(env, nclass_stmt, self->db, argv[0], &out);

  if (r != napi_ok || out == NULL) {
    goto end;
  }

end:
  return nsql_return(env, r, out);
}

static napi_value nsql_database_get_db_filename(napi_env env,
                                                napi_callback_info ctx) {
  struct nsql_database *self;
  const char *str;
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

  if (self->db != NULL) {
    str = sqlite3_db_filename(self->db, "main");
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
