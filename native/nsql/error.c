#include <stdbool.h>
#include <stdlib.h>

#include <node_api.h>
#include <sqlite3.h>

#include "dprintf.h"
#include "error.h"

static const char *nsql_sqlite_error_name(int code);
static const char *nsql_sqlite_error_name_step(int code);

static const char *nsql_error_message(napi_env env) {
  const napi_extended_error_info *info;
  napi_status r;

  r = napi_get_last_error_info(env, &info);

  if (r != napi_ok) {
    return "Undiagnosable error (failed to get error)";
  }

  if (info == NULL) {
    return "Undiagnosable error (no error info)";
  }

  if (info->error_message == NULL) {
    return "Undiagnosable error (no error message)";
  }

  return info->error_message;
}

void nsql_report_error_(napi_env env, napi_status r, const char *file,
                        int line) {
  nsql_dprintf("%s: %s:%i: (%i) %s\n", __func__, file, line, r,
               nsql_error_message(env));
}

_Noreturn void nsql_fatal_error_(napi_env env, napi_status r, const char *file,
                                 int line) {
  nsql_dprintf("%s: %s:%i: (%i) %s\n", __func__, file, line, r,
               nsql_error_message(env));
  abort();
}

napi_value nsql_propagate_error_(napi_env env) {
  bool pending;
  napi_status r;

  r = napi_is_exception_pending(env, &pending);

  if (r != napi_ok) {
    nsql_fatal_error(env, r);
  }

  if (!pending) {
    r = napi_throw_error(env, NULL, nsql_error_message(env));

    if (r != napi_ok) {
      nsql_fatal_error(env, r);
    }
  }

  return NULL;
}

napi_status nsql_throw_oom(napi_env env) {
  return napi_throw_error(env, "ERR_MEMORY_ALLOCATION_FAILED", "Out of memory");
}

napi_status nsql_throw_sqlite_error(napi_env env, int code, sqlite3 *db) {
  const char *msg;
  napi_status r;

  if (db != NULL) {
    /* Throw a specific error string based on the connection's last error */
    msg = sqlite3_errmsg(db);
  } else {
    /* No connection object available, throw a generic code description */
    msg = sqlite3_errstr(code);
  }

  r = napi_throw_error(env, nsql_sqlite_error_name(code), msg);

  if (r != napi_ok) {
    nsql_report_error(env, r);
  }

  return r;
}

_Noreturn void nsql_fatal_sqlite_error_(int code, const char *file, int line) {
  nsql_dprintf("%s: %s:%i: (%i) %s\n", __func__, file, line, code,
               nsql_sqlite_error_name(code));
  abort();
}

/* Ideally we would use sqlite3ErrName, but that's an internal function. */

static const char *nsql_sqlite_error_name(int code) {
  const char *str;

  str = nsql_sqlite_error_name_step(code);

  if (str != NULL) {
    return str;
  }

  return nsql_sqlite_error_name_step(code & 0xFF);
}

static const char *nsql_sqlite_error_name_step(int code) {
  switch (code) {
  case SQLITE_OK:
    return "SQLITE_OK";

  case SQLITE_ERROR:
    return "SQLITE_ERROR";

  case SQLITE_ERROR_SNAPSHOT:
    return "SQLITE_ERROR_SNAPSHOT";

  case SQLITE_INTERNAL:
    return "SQLITE_INTERNAL";

  case SQLITE_PERM:
    return "SQLITE_PERM";

  case SQLITE_ABORT:
    return "SQLITE_ABORT";

  case SQLITE_ABORT_ROLLBACK:
    return "SQLITE_ABORT_ROLLBACK";

  case SQLITE_BUSY:
    return "SQLITE_BUSY";

  case SQLITE_BUSY_RECOVERY:
    return "SQLITE_BUSY_RECOVERY";

  case SQLITE_BUSY_SNAPSHOT:
    return "SQLITE_BUSY_SNAPSHOT";

  case SQLITE_LOCKED:
    return "SQLITE_LOCKED";

  case SQLITE_LOCKED_SHAREDCACHE:
    return "SQLITE_LOCKED_SHAREDCACHE";

  case SQLITE_NOMEM:
    return "SQLITE_NOMEM";

  case SQLITE_READONLY:
    return "SQLITE_READONLY";

  case SQLITE_READONLY_RECOVERY:
    return "SQLITE_READONLY_RECOVERY";

  case SQLITE_READONLY_CANTINIT:
    return "SQLITE_READONLY_CANTINIT";

  case SQLITE_READONLY_ROLLBACK:
    return "SQLITE_READONLY_ROLLBACK";

  case SQLITE_READONLY_DBMOVED:
    return "SQLITE_READONLY_DBMOVED";

  case SQLITE_READONLY_DIRECTORY:
    return "SQLITE_READONLY_DIRECTORY";

  case SQLITE_INTERRUPT:
    return "SQLITE_INTERRUPT";

  case SQLITE_IOERR:
    return "SQLITE_IOERR";

  case SQLITE_IOERR_READ:
    return "SQLITE_IOERR_READ";

  case SQLITE_IOERR_SHORT_READ:
    return "SQLITE_IOERR_SHORT_READ";

  case SQLITE_IOERR_WRITE:
    return "SQLITE_IOERR_WRITE";

  case SQLITE_IOERR_FSYNC:
    return "SQLITE_IOERR_FSYNC";

  case SQLITE_IOERR_DIR_FSYNC:
    return "SQLITE_IOERR_DIR_FSYNC";

  case SQLITE_IOERR_TRUNCATE:
    return "SQLITE_IOERR_TRUNCATE";

  case SQLITE_IOERR_FSTAT:
    return "SQLITE_IOERR_FSTAT";

  case SQLITE_IOERR_UNLOCK:
    return "SQLITE_IOERR_UNLOCK";

  case SQLITE_IOERR_RDLOCK:
    return "SQLITE_IOERR_RDLOCK";

  case SQLITE_IOERR_DELETE:
    return "SQLITE_IOERR_DELETE";

  case SQLITE_IOERR_NOMEM:
    return "SQLITE_IOERR_NOMEM";

  case SQLITE_IOERR_ACCESS:
    return "SQLITE_IOERR_ACCESS";

  case SQLITE_IOERR_CHECKRESERVEDLOCK:
    return "SQLITE_IOERR_CHECKRESERVEDLOCK";

  case SQLITE_IOERR_LOCK:
    return "SQLITE_IOERR_LOCK";

  case SQLITE_IOERR_CLOSE:
    return "SQLITE_IOERR_CLOSE";

  case SQLITE_IOERR_DIR_CLOSE:
    return "SQLITE_IOERR_DIR_CLOSE";

  case SQLITE_IOERR_SHMOPEN:
    return "SQLITE_IOERR_SHMOPEN";

  case SQLITE_IOERR_SHMSIZE:
    return "SQLITE_IOERR_SHMSIZE";

  case SQLITE_IOERR_SHMLOCK:
    return "SQLITE_IOERR_SHMLOCK";

  case SQLITE_IOERR_SHMMAP:
    return "SQLITE_IOERR_SHMMAP";

  case SQLITE_IOERR_SEEK:
    return "SQLITE_IOERR_SEEK";

  case SQLITE_IOERR_DELETE_NOENT:
    return "SQLITE_IOERR_DELETE_NOENT";

  case SQLITE_IOERR_MMAP:
    return "SQLITE_IOERR_MMAP";

  case SQLITE_IOERR_GETTEMPPATH:
    return "SQLITE_IOERR_GETTEMPPATH";

  case SQLITE_IOERR_CONVPATH:
    return "SQLITE_IOERR_CONVPATH";

  case SQLITE_CORRUPT:
    return "SQLITE_CORRUPT";

  case SQLITE_CORRUPT_VTAB:
    return "SQLITE_CORRUPT_VTAB";

  case SQLITE_NOTFOUND:
    return "SQLITE_NOTFOUND";

  case SQLITE_FULL:
    return "SQLITE_FULL";

  case SQLITE_CANTOPEN:
    return "SQLITE_CANTOPEN";

  case SQLITE_CANTOPEN_NOTEMPDIR:
    return "SQLITE_CANTOPEN_NOTEMPDIR";

  case SQLITE_CANTOPEN_ISDIR:
    return "SQLITE_CANTOPEN_ISDIR";

  case SQLITE_CANTOPEN_FULLPATH:
    return "SQLITE_CANTOPEN_FULLPATH";

  case SQLITE_CANTOPEN_CONVPATH:
    return "SQLITE_CANTOPEN_CONVPATH";

  case SQLITE_PROTOCOL:
    return "SQLITE_PROTOCOL";

  case SQLITE_EMPTY:
    return "SQLITE_EMPTY";

  case SQLITE_SCHEMA:
    return "SQLITE_SCHEMA";

  case SQLITE_TOOBIG:
    return "SQLITE_TOOBIG";

  case SQLITE_CONSTRAINT:
    return "SQLITE_CONSTRAINT";

  case SQLITE_CONSTRAINT_UNIQUE:
    return "SQLITE_CONSTRAINT_UNIQUE";

  case SQLITE_CONSTRAINT_TRIGGER:
    return "SQLITE_CONSTRAINT_TRIGGER";

  case SQLITE_CONSTRAINT_FOREIGNKEY:
    return "SQLITE_CONSTRAINT_FOREIGNKEY";

  case SQLITE_CONSTRAINT_CHECK:
    return "SQLITE_CONSTRAINT_CHECK";

  case SQLITE_CONSTRAINT_PRIMARYKEY:
    return "SQLITE_CONSTRAINT_PRIMARYKEY";

  case SQLITE_CONSTRAINT_NOTNULL:
    return "SQLITE_CONSTRAINT_NOTNULL";

  case SQLITE_CONSTRAINT_COMMITHOOK:
    return "SQLITE_CONSTRAINT_COMMITHOOK";

  case SQLITE_CONSTRAINT_VTAB:
    return "SQLITE_CONSTRAINT_VTAB";

  case SQLITE_CONSTRAINT_FUNCTION:
    return "SQLITE_CONSTRAINT_FUNCTION";

  case SQLITE_CONSTRAINT_ROWID:
    return "SQLITE_CONSTRAINT_ROWID";

  case SQLITE_MISMATCH:
    return "SQLITE_MISMATCH";

  case SQLITE_MISUSE:
    return "SQLITE_MISUSE";

  case SQLITE_NOLFS:
    return "SQLITE_NOLFS";

  case SQLITE_AUTH:
    return "SQLITE_AUTH";

  case SQLITE_FORMAT:
    return "SQLITE_FORMAT";

  case SQLITE_RANGE:
    return "SQLITE_RANGE";

  case SQLITE_NOTADB:
    return "SQLITE_NOTADB";

  case SQLITE_ROW:
    return "SQLITE_ROW";

  case SQLITE_NOTICE:
    return "SQLITE_NOTICE";

  case SQLITE_NOTICE_RECOVER_WAL:
    return "SQLITE_NOTICE_RECOVER_WAL";

  case SQLITE_NOTICE_RECOVER_ROLLBACK:
    return "SQLITE_NOTICE_RECOVER_ROLLBACK";

  case SQLITE_WARNING:
    return "SQLITE_WARNING";

  case SQLITE_WARNING_AUTOINDEX:
    return "SQLITE_WARNING_AUTOINDEX";

  case SQLITE_DONE:
    return "SQLITE_DONE";

  default:
    return NULL;
  }
}
