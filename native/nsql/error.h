#pragma once

#include <node_api.h>
#include <sqlite3.h>

#if __STDC_VERSION__ < 201112L
#define _Noreturn
#endif

#ifndef NDEBUG
/*
 * Report the origin of an N-API error. Not present in release builds.
 */
#define nsql_report_error(env, r) nsql_report_error_(env, r, __FILE__, __LINE__)

/*
 * Report an N-API error that occurred in a situation where recovery is
 * impossible (typically in the course of resource cleanup). Does not return
 * and causes the host process to abort.
 */
#define nsql_fatal_error(env, r) nsql_fatal_error_(env, r, __FILE__, __LINE__)

/*
 * Report an SQLite error that occurred in a situation where recovery is
 * impossible (typically in the course of resource cleanup). Does not return
 * and causes the host process to abort.
 */
#define nsql_fatal_sqlite_error(code)                                          \
  nsql_fatal_sqlite_error_(code, __FILE__, __LINE__)
#else
#define nsql_report_error(env, r)
#define nsql_fatal_error(env, r) nsql_fatal_error_(env, r, NULL, 0)
#define nsql_fatal_sqlite_error(code) nsql_fatal_sqlite_error_(code, NULL, 0)
#endif

/*
 * Propagate N-API errors as JavaScript exceptions. Must be called on return
 * from an NSQL entry point. `env` is the N-API environment, `status` is the
 * `napi_status` propagated from downstream function calls, and `result` is
 * the `napi_value` to return if `status` is `napi_ok`.
 *
 * See DEVELOPMENT.md for further details.
 */
#define nsql_return(env, status, result)                                       \
  ((status) == napi_ok ? (result) : nsql_propagate_error_(env));

void nsql_report_error_(napi_env env, napi_status r, const char *file,
                        int line);

_Noreturn void nsql_fatal_error_(napi_env env, napi_status r, const char *file,
                                 int line);

napi_value nsql_propagate_error_(napi_env env);

/*
 * Attempt to throw a JavaScript exception indicating an out-of-memory error.
 */
napi_status nsql_throw_oom(napi_env env);

/*
 * Throw a JavaScript exception indicating an SQLite error. The exception will
 * have a `code` field containing the name of the error code that was passed in
 * to this function.
 *
 * `db` is an optional pointer to an SQLite database connection. If this is not
 * NULL then `sqlite3_errmsg()` will be called to obtain a detailed description
 * of the last error to occur on this database connection. Otherwise a generic
 * description for the error code will be thrown.
 */
napi_status nsql_throw_sqlite_error(napi_env env, int code, sqlite3 *db);

_Noreturn void nsql_fatal_sqlite_error_(int code, const char *file, int line);
