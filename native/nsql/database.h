#pragma once

#include <node_api.h>

/*
 * Define and return a JavaScript constructor function that can be used to
 * create `Database` objects.
 */
napi_status nsql_database_define_class(napi_env env, napi_value *nclass);
