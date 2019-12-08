#include <node_api.h>
#include <sqlite3.h>

#include "database.h"
#include "dprintf.h"
#include "error.h"

static void nsql_log(void *ctx, int code, const char *msg);

static void nsql_log(void *ctx, int code, const char *msg) {
  nsql_dprintf("%s: (%i) %s\n", __func__, code, msg);
}

napi_value nsql_init(napi_env env, napi_value exports) {
  napi_value nclass;
  int r;

  sqlite3_config(SQLITE_CONFIG_LOG, nsql_log, NULL);

  r = nsql_database_define_class(env, &nclass);

  return nsql_return(env, r, nclass);
}

NAPI_MODULE(sqlite, nsql_init);
