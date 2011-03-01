/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_BLOB_H_
#define SRC_FB_BINDINGS_BLOB_H_

#include <v8.h>
//#include <node.h>
//#include <node_buffer.h>
#include <ibase.h>
#include "./fb-bindings.h"
#include "./fb-bindings-fbeventemitter.h"
// #include "./fb-bindings-connection.h"


class FBblob : public FBEventEmitter {

public:
 
  static Persistent<FunctionTemplate> constructor_template;
  
  static void
  Initialize (v8::Handle<v8::Object> target);
  
 protected:
  static Handle<Value>
  New(const Arguments& args);
  
  static Handle<Value>
  ReadSync(const Arguments& args);
  
  
  struct read_request {
     Persistent<Function> callback;
     FBblob *blob;
     size_t length;
  };
  
  static int EIO_After_Read(eio_req *req);
  
  static int EIO_Read(eio_req *req);
  
  
  static Handle<Value>
  Read(const Arguments& args);
  
  FBblob(ISC_QUAD *id, Connection *conn);
  ~FBblob();
  
 private: 
  ISC_QUAD blob_id; 
  Connection *connection;
  
};  

#endif
