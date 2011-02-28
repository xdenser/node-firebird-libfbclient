/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_BLOB_H_
#define SRC_FB_BINDINGS_BLOB_H_

#include <ibase.h>
#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include "./fb-bindings-fbeventemitter.h"


using namespace v8;
using namespace node;


class Blob : public FBEventEmitter {
 public:
  static void
  Initialize (v8::Handle<v8::Object> target);
  
 protected:
  static Handle<Value>
  New(const Arguments& args);
  
  static Handle<Value>
  ReadSync(const Arguments& args);
  
  
  struct read_request {
     Persistent<Function> callback;
     Blob *blob;
     size_t length;
  };
  
  static int EIO_After_Read(eio_req *req);
  
  static int EIO_Read(eio_req *req);
  
  
  static Handle<Value>
  Read(const Arguments& args);
  
  
  
 private: 
  ISC_QUAD blob_id; 
  isc_db_handle db;
  isc_tr_handle trans;
  
}  

#endif