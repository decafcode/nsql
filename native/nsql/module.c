#include <node_api.h>
#include <sqlite3.h>

#include "dprintf.h"

static void nsql_log(void *ctx, int code, const char *msg);

static void nsql_log(void *ctx, int code, const char *msg) {
  nsql_dprintf("%s: (%i) %s\n", __func__, code, msg);
}

napi_value nsql_init(napi_env env, napi_value exports) {
  sqlite3_config(SQLITE_CONFIG_LOG, nsql_log, NULL);

  return NULL;
}

NAPI_MODULE(sqlite, nsql_init);
