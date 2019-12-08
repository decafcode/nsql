#pragma once

#include <stddef.h>

#include <node_api.h>

/*
 * Create a dynamically-allocated C string from an N-API string value. The
 * caller must verify beforehand that the value is actually a string.
 *
 * The allocated string is returned through the `out` parameter. If memory
 * allocation fails then `*out` will be set to NULL and a JavaScript exception
 * will be thrown.
 *
 * If `out_nbytes` is non-NULL then the number of UTF-8 bytes in the string
 * will be returned in `*out_nbytes`. This count does not include the NUL
 * terminator.
 */
napi_status nsql_get_string(napi_env env, napi_value value, char **out,
                            size_t *out_nbytes);
