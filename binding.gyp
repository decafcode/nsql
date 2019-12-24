# The documentation available for node-gyp is perhaps not as comprehensive
# and well-organized as it could be. The following articles were referenced
# during the construction of this file:
#
# https://gyp.gsrc.io/docs/LanguageSpecification.md
# https://gyp.gsrc.io/docs/UserDocumentation.md
# https://web.archive.org/web/20160430013351/https://n8.io/converting-a-c-library-to-gyp/

{
  'target_defaults': {
    'configurations': {
      'Debug': {},
      'Release': {
        'defines': ['NDEBUG'],
      },
    },
  },
  'targets': [
    {
      'target_name': 'sqlite',
      'product_prefix': 'lib',
      'type': 'static_library',
      'conditions': [
        # Disable SQLite warnings for GCC-style (i.e. non-MSVC) compilers
        ['OS!="win"', {
          'cflags': [
            '-Wno-cast-function-type',
            '-Wno-implicit-fallthrough',
          ],
        }],
      ],
      'defines': [
        # Recommendations from https://www.sqlite.org/compile.html
        # a/o 2019-12-01
        'SQLITE_DQS=0',
        'SQLITE_THREADSAFE=0',
        'SQLITE_DEFAULT_MEMSTATUS=0',
        'SQLITE_DEFAULT_WAL_SYNCHRONOUS=1',
        'SQLITE_LIKE_DOESNT_MATCH_BLOBS',
        'SQLITE_MAX_EXPR_DEPTH=0',
        'SQLITE_OMIT_DECLTYPE',
        'SQLITE_OMIT_DEPRECATED',
        'SQLITE_OMIT_PROGRESS_CALLBACK',
        'SQLITE_OMIT_SHARED_CACHE',
        'SQLITE_USE_ALLOCA',

        # Executive decisions
        'SQLITE_DEFAULT_FOREIGN_KEYS=1',
        'SQLITE_ENABLE_STAT4'
      ],
      'include_dirs': [
        'native/sqlite',
      ],
      'sources': [
        'native/sqlite/sqlite3.c',
      ],
    },
    {
      'target_name': 'nsql',
      'dependencies': [
        'sqlite'
      ],
      'defines': [
        'NAPI_EXPERIMENTAL'
      ],
      'include_dirs': [
        'native/sqlite',
        'native/nsql',
      ],
      'sources': [
        'native/nsql/bind.c',
        'native/nsql/database.c',
        'native/nsql/dprintf.c',
        'native/nsql/error.c',
        'native/nsql/module.c',
        'native/nsql/result.c',
        'native/nsql/statement.c',
        'native/nsql/str.c',
      ]
    }
  ]
}
