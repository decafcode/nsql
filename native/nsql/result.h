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

/*
 * Extract the next row of an SQLite result set and convert it into a JavaScript
 * object.
 *
 * Requires a C array of JavaScript strings representing the result set's
 * column names; this can be constructed by calling `nsql_result_get_columns()`.
 */
napi_status nsql_result_get_row(napi_env env, sqlite3_stmt *stmt,
                                napi_value *cols, size_t ncols,
                                napi_value *out);
