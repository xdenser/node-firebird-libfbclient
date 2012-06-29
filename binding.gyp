{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [ './src/fb-bindings.cc', './src/fb-bindings-blob.cc',
                   './src/fb-bindings-fbresult.cc',
                   './src/fb-bindings-connection.cc','./src/fb-bindings-eventblock.cc',
                   './src/fb-bindings-fbeventemitter.cc', 
                   './src/fb-bindings-statement.cc' ],
      'include_dirs': [
          '/usr/include/'
      ],
      'libraries': [ '-L/usr/lib -lfbclient' ]
    }
  ]
}