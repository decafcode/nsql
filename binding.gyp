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
      'target_name': 'nsql',
      'include_dirs': [
        'native/nsql',
      ],
      'sources': [
        'native/nsql/module.c',
      ]
    }
  ]
}
