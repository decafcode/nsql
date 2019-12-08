#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "dprintf.h"
#include "error.h"
#include "macros.h"
#include "str.h"

struct nsql_database {
  sqlite3 *db;
};

static napi_value nsql_database_constructor(napi_env env,
                                            napi_callback_info ctx);

static void nsql_database_destructor(napi_env env, void *ptr, void *hint);

static napi_value nsql_database_close(napi_env env, napi_callback_info ctx);

static const napi_property_descriptor nsql_database_desc[] = {
    {.utf8name = "close", .method = nsql_database_close}};

napi_status nsql_database_define_class(napi_env env, napi_value *out) {
  napi_status r;

  assert(out != NULL);

  nsql_dprintf("%s\n", __func__);

  r = napi_define_class(env, "Database", NAPI_AUTO_LENGTH,
                        nsql_database_constructor, NULL,
                        countof(nsql_database_desc), nsql_database_desc, out);

  if (r != napi_ok) {
    nsql_report_error(env, r);

    goto end;
  }

end:
  return r;
}

static napi_value nsql_database_constructor(napi_env env,
                                            napi_callback_info ctx) {
  char *uri;
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
  r = napi_get_cb_info(env, ctx, &argc, argv, &nself, NULL);

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

  sqlr = sqlite3_open_v2(uri, &self->db,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

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
