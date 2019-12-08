#include <node_api.h>

napi_value nsql_init(napi_env env, napi_value exports) { return NULL; }

NAPI_MODULE(sqlite, nsql_init);
