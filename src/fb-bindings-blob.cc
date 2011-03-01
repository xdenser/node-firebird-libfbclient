/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#include <v8.h> 
#include <node.h>
#include "./fb-bindings-blob.h"

 v8::Persistent<v8::FunctionTemplate> FBblob::constructor_template;

void FBblob::Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(FBEventEmitter::constructor_template);
        
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(FBEventEmitter::constructor_template);
    constructor_template->SetClassName(String::NewSymbol("FBblob"));

    Local<ObjectTemplate> instance_template =
        constructor_template->InstanceTemplate();
        
    NODE_SET_PROTOTYPE_METHOD(t, "readSync", ReadSync);
    NODE_SET_PROTOTYPE_METHOD(t, "read", Read);


    instance_template->SetInternalFieldCount(1);
    
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    instance_t->SetAccessor(String::NewSymbol("inAsyncCall"),InAsyncGetter);
    
    target->Set(String::NewSymbol("FBblob"), t->GetFunction());

  }
  
Handle<Value> FBblob::New(const Arguments& args)
  {
    HandleScope scope;
    
    REQ_EXT_ARG(0, js_quad);
    REQ_EXT_ARG(1, js_connection);
    
    ISC_QUAD *quad;
    quad = static_cast<ISC_QUAD *>(js_quad->Value());
    Connection  *conn;
    conn = static_cast<Connection *>(js_connection->Value()); 
    
    FBblob *res = new FBblob(quad, conn);
    res->Wrap(args.This());

    return args.This();
  }
  
Handle<Value> FBblob::ReadSync(const Arguments& args)
  {
    return Undefined();
  }
  
int FBblob::EIO_After_Read(eio_req *req)
  {
    return 0;
  }
  
int FBblob::EIO_Read(eio_req *req)
  {
    return 0;
  }

Handle<Value> FBblob::Read(const Arguments& args)
  {
    return Undefined();
  }
  
FBblob::FBblob(ISC_QUAD *id, Connection *conn): FBEventEmitter () 
  {
    blob_id = *id; 
    connection = conn;
  }

FBblob::~FBblob()
  {

  }    
