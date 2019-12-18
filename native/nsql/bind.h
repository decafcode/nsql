#pragma once

#include <stdbool.h>

#include <node_api.h>
#include <sqlite3.h>

napi_status nsql_bind(napi_env env, napi_value values, sqlite3_stmt *stmt,
                      bool *ok);
