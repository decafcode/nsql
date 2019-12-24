#pragma once

#include <stddef.h>

#include <node_api.h>
#include <sqlite3.h>

/*
 * Extract a result set's column names as a C array of `napi_value`s. The array
 * itself must be `free()`d after use.
 */
napi_status nsql_result_get_columns(napi_env env, sqlite3_stmt *stmt,
                                    napi_value **out_cols, size_t *out_ncols);
