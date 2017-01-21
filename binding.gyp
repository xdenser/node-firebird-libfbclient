{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [ './src/fb-bindings.cc', './src/fb-bindings-blob.cc',
                   './src/fb-bindings-fbresult.cc',
                   './src/fb-bindings-connection.cc','./src/fb-bindings-eventblock.cc',
                   './src/fb-bindings-fbeventemitter.cc',
                   './src/fb-bindings-statement.cc',
		   './src/fb-bindings-transaction.cc' ],
      'include_dirs': [
          '<(module_root_dir)/fb/include',
          "<!(node -e \"require('nan')\")"
      ],
      "conditions" : [
            [
                'OS=="linux"', {
                    'libraries': [ '-lfbclient' ]
                }
            ],
            [
                'OS=="win" and target_arch=="ia32"', {
                  "libraries" : [
                      '<(module_root_dir)/fb/lib/fbclient_ms.lib'
                  ]
                }
            ],
            [
                'OS=="win" and target_arch=="x64"', {
                  "libraries" : [
                      '<(module_root_dir)/fb/lib64/fbclient_ms.lib'
                  ]
                }
            ],
            [
              'OS=="mac"', {
                "link_settings" : {
                  "libraries": ['-L/Library/Frameworks/Firebird.framework/Libraries/', '-lfbclient']
                }
              }
            ]
      ]
    }
  ]
}
