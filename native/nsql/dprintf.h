#pragma once

#ifdef NDEBUG
#define nsql_dprintf(...)
#else
/*
 * Write a debug message to `stderr` if the `NSQL_VERBOSE`
 * environment variable is present. Not present in release
 * builds.
 */
void nsql_dprintf(const char *fmt, ...);
#endif
