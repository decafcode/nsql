#pragma once

#include <node_api.h>
#include <sqlite3.h>

/*
 * Define and return a JavaScript constructor function that can be used to
 * create `Statement` objects. This constructor should not be invoked directly,
 * but should instead be retained for use with `nsql_statement_prepare()`.
 */
napi_status nsql_statement_define_class(napi_env env, napi_value *out);
