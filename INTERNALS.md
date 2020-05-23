# Introduction

This file describes various aspects of the `@decafcode/sqlite` project's
internals. End users of this library won't find anything useful here.

Internally this project uses the name "NSQL" for C namespacing purposes, and we
will use that name here as well for brevity's sake.

# Development Environment

Developing with Visual Studio Code is strongly recommended. The development
environment can be set up as follows:

1. Initialize the Git submodule containing the SQLite amalgamation:
   ```
   $ git submodule init
   $ git submodule update
   ```
2. Run `npm install` to download dependencies.
3. Run `npx node-gyp build` to force local compilation of the native code. You
   will need to have the following development tools installed on your local
   machine:
   - Python 3 (for `node-gyp`)
   - On Linux:
     - GNU Make
     - A GCC-compatible C compiler
     - A GCC-compatible C++ compiler (for the linking step)
   - On Windows:
     - The development tools from Visual Studio 2015. Consult
       [this article](https://spin.atomicobject.com/2019/03/27/node-gyp-windows/)
       for further details.
   - On macOS:
     - The XCode Command Line Tools, which can be installed by typing
       `xcode-select --install` at a command-line prompt.
4. Run `npm test` to run the test suite and confirm that your build works
   correctly.
5. Run `npm run headers` to download our target version of the Node.js SDK to
   `node-gyp`'s cache. This cache is located in your home directory, so you
   should only need to do this once.
6. Open your Git checkout in Visual Studio Code and accept all of the workspace
   extension recommendations.

This project currently uses version 12.13.1 of the Node.js SDK for Visual Studio
Code's C/C++ code completion. `node-gyp-build` will download and compile against
the SDK version that corresponds to your local machine's global Node.js
installation (or your currently-selected environment in `nvm`, if applicable).
This is a little unfortunate from a build predictability standpoint, but N-API
aims to maintain forward binary compatibility with all future releases of
Node.js, so the exact SDK version is less important than it is for extensions
that use the traditional Node.js C++ extension API.

That being said, we do currently rely on un-stabilized entry points in N-API,
specifically BigInt support. Hopefully these interfaces will soon be declared
stable in their current state.

The NSQL repository includes configuration for Visual Studio Code to integrate
with our preferred source code formatting tools and their default formatting
settings. Source code files will be automatically re-formatted to comply with
these settings every time you save.

# Environment Variables

Debug builds of NSQL check whether an `NSQL_VERBOSE` environment variable
exists. If so, they will emit debug messages to `stderr`. Currently this
consists of object lifecycle events and calls to the `SQLITE_LOG` callback.

Please note that any behaviors that are exclusive to debug builds are not
subject to semantic versioning guarantees.

# Error Handling

A robust program written in an unmanaged language like C needs to manage its
system resources in a transactional manner: if any operation undertaken in the
course of its internal processing fails then the resources allocated by the
intermediate processing steps up to that point need to be deallocated in order
to prevent memory or any other operating system resources from leaking.

This presents above-average difficulties for extensions called from an N-API
runtime, since the programmer must deal with three error reporting mechanisms
simultaneously:

1. Unsuccessful `napi_status` return codes, which may be returned in response to
   any call back into the N-API runtime,
2. JavaScript exceptions, which the native-code extension may wish to throw in
   response to various error conditions, and
3. The error reporting mechanism used by the library being wrapped, if
   applicable.

(Note that N-API errors should only occur as a result of unrecoverable errors in
the runtime such as memory exhaustion or as a result of programming errors.
Situations like incorrect parameter types should be checked explicitly in NSQL
code instead of relying on N-API to catch them).

NSQL deals with these error sources through a combination of helper functions
and programming conventions. To illustrate these features, we must categorize
the functions in this project into three classes:

1. **Destructors**, which are called by the host runtime when a native-code
   object should release its operating system resources,
2. **Entry points**, which are invoked by the host runtime to service JavaScript
   calls into native code, and
3. **Internal functions**, which are functions that are called by NSQL itself.

## Destructors

Destructors are not permitted to fail, since they may be called while some other
exception handling procedure is already in progress. Since we cannot propagate
any failures encountered during resource cleanup back to the library client we
have to react to any such errors by aborting the entire Node.js process.

In practice most C APIs also abide by this restriction and do not signal errors
in response to their cleanup and deallocation functions being called, and the
only situations in which SQLite returns an error in response to a cleanup call
is if its API is being mis-used, in which case something has gone seriously
wrong some time ago and we are already past the point where we can recover from
it anyway. So really this shouldn't be any more controversial than aborting the
process in response to a memory access violation.

Although destructors are usually invoked by the JavaScript runtime, NSQL code
may also call them directly as part of a function's RAII-like cleanup block. We
directly re-use the destructor functions that we expose to N-API instead of
defining our own internal cleanup and disposal functions because our native-code
objects may contain `napi_ref`s which need to be released, and an `napi_env`
reference is needed to release them.

## Entry Points

Entry points are required to return an `napi_value` to the runtime. No path is
provided for `napi_status` errors to be propagated to the calling application,
so if we are returning due to an N-API error condition then this needs to be
converted into a JavaScript exception. This is handled by the `nsql_return`
macro, and all entry point functions must return by evaluating `nsql_return()`
and returning the result; it will return the entry point's `napi_value` result
if there is no `napi_status` error, and it will return `NULL` and throw a
JavaScript exception if there is an `napi_status` error.

## Internal Functions

Internal functions must always return an `napi_status` to provide a path for
`napi_status` errors to propagate out. However, as mentioned previously, it is
possible for other kinds of errors to occur as well as `napi_status` errors. For
functions where this is a possibility those errors must be reported through
various out-parameters.

To give an example, if a function allocates a C string (which could fail due to
insufficient memory), then an out-parameter would be used to return this C
string and this out-parameter would be set to `NULL` if the allocation fails.
The allocating function itself is responsible for raising a JavaScript exception
in this situation, and its caller is responsible for checking both the returned
`napi_status` and the out-parameter's value to see if either kind of error has
occurred.

## General Notes

Direct calls to N-API should of course be checked to see if they returned an
`napi_status` that isn't `napi_ok`. If an error is reported then the
`nsql_report_error` macro should also be invoked. In release builds this macro
does nothing, but in debug builds it calls an error reporting function that
queries the N-API runtime for extended error information and prints this
information to the debug log. Unlike `napi_return` this function reports the
error at the point where it originated, not the point at which it exited NSQL
back into the host application, so this makes debugging of internal errors
easier.
