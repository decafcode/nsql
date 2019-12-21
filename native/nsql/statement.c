#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "dprintf.h"
#include "error.h"
#include "macros.h"
#include "statement.h"

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

static const napi_property_descriptor nsql_statement_desc[] = {};

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
