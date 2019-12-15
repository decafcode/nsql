#pragma once

#include <node_api.h>
#include <sqlite3.h>

/*
 * Define and return a JavaScript constructor function that can be used to
 * create `Statement` objects. This constructor should not be invoked directly,
 * but should instead be retained for use with `nsql_statement_prepare()`.
 */
napi_status nsql_statement_define_class(napi_env env, napi_value *out);

/*
 * Construct a prepared statement and then wrap it in a newly-constructed
 * JavaScript `Statement` object. Requires a constructor function that was
 * previously defined by `nsql_statement_define_class()`; this should be passed
 * in the `nclass` parameter.
 */
napi_status nsql_statement_prepare(napi_env env, napi_value nclass, sqlite3 *db,
                                   napi_value nsql, napi_value *out);
