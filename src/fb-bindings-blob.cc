/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#define BUILDING_NODE_EXTENSION 1
#include <stdlib.h>
#include "./fb-bindings-blob.h"
#include "./fb-bindings-connection.h"

v8::Persistent<v8::FunctionTemplate> FBblob::constructor_template;
char FBblob::err_message[MAX_ERR_MSG_LEN];

void FBblob::Initialize (v8::Handle<v8::Object> target)
  {
    HandleScope scope;
    
    Local<FunctionTemplate> t = FunctionTemplate::New(FBblob::New);

    t->Inherit(FBEventEmitter::constructor_template);
        
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(FBEventEmitter::constructor_template);
    constructor_template->SetClassName(String::NewSymbol("FBblob"));

    Local<ObjectTemplate> instance_template =
        constructor_template->InstanceTemplate();
        
    NODE_SET_PROTOTYPE_METHOD(t, "_readSync", ReadSync);
    NODE_SET_PROTOTYPE_METHOD(t, "_read", Read);
    NODE_SET_PROTOTYPE_METHOD(t, "_openSync", OpenSync);
    NODE_SET_PROTOTYPE_METHOD(t, "_closeSync", CloseSync);
    NODE_SET_PROTOTYPE_METHOD(t, "_writeSync", WriteSync);
    NODE_SET_PROTOTYPE_METHOD(t, "_write", Write);


    instance_template->SetInternalFieldCount(1);
    
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    instance_t->SetAccessor(String::NewSymbol("inAsyncCall"),InAsyncGetter);
    instance_t->SetAccessor(String::NewSymbol("isReadable"),IsReadGetter);
    
    target->Set(String::NewSymbol("FBblob"), t->GetFunction());

  }

bool FBblob::HasInstance(v8::Handle<v8::Value> val)
   { 
       if (!val->IsObject()) return false;
       v8::Local<v8::Object> obj = val->ToObject();
/*
  if (obj->GetIndexedPropertiesExternalArrayDataType() == kExternalUnsignedByteArray)
    return true;
*/
       if (constructor_template->HasInstance(obj))
        return true;

       return false;
   }  
void FBblob::getId(ISC_QUAD* Idp)
  {
    *Idp = blob_id;
  }    
  
Handle<Value> FBblob::New(const Arguments& args)
  {
    HandleScope scope;

    ISC_QUAD *quad = NULL;
    Connection  *conn = NULL;
    ISC_STATUS_ARRAY status;
    
    if((args.Length() > 0) && !args[0]->IsNull()){
      REQ_EXT_ARG(0, js_quad);
      quad = static_cast<ISC_QUAD *>(js_quad->Value());
    }
    
    if(args.Length() > 1) {
      REQ_EXT_ARG(1, js_connection);
      conn = static_cast<Connection *>(js_connection->Value()); 
    }

    status[1] = 0;
    FBblob *res = new FBblob(quad, conn, status);
    if(status[1]) 
     return ThrowException(Exception::Error(
         String::Concat(String::New("In FBblob::New - "),ERR_MSG_STAT(status, FBblob))));
         
    res->Wrap(args.This());

    return args.This();
  }
  
Handle<Value> FBblob::ReadSync(const Arguments& args)
  {
    HandleScope scope;
    
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(args.This());
    
    if (!Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::Error(
                String::New("First argument needs to be a buffer")));
    }
    
    Local<Object> buffer_obj = args[0]->ToObject();
    char *buffer_data = Buffer::Data(buffer_obj);
    size_t buffer_length = Buffer::Length(buffer_obj);
    //printf("buffer len %d\n",buffer_length);
    
    ISC_STATUS_ARRAY status;
    int res = blob->read(status, buffer_data, (unsigned short) buffer_length);
    if(res==-1) {
       return ThrowException(Exception::Error(
         String::Concat(String::New("In FBblob::_readSync - "),ERR_MSG_STAT(status, FBblob))));
    }
    
    return scope.Close(Integer::New(res));
  }
  
void FBblob::EIO_After_Read(uv_work_t *req)
  {
   // uv_unref(uv_default_loop());
    HandleScope scope;
    struct rw_request *r_req = (struct rw_request *)(req->data);
	delete req;
    Local<Value> argv[3];
    int argc;
    
    Buffer *slowBuffer = Buffer::New(r_req->buffer, (size_t) r_req->length);
    Local<Object> global = Context::GetCurrent()->Global();
    Local<Function> bufferConstructor =  Local<Function>::Cast(global->Get(String::New("Buffer")));
    Handle<Value> cArgs[3] = {slowBuffer->handle_, Integer::New(r_req->length), Integer::New(0) };
    
    if(r_req->res!=-1)
    {
      argv[0] = Local<Value>::New(Null());
      argv[1] = bufferConstructor->NewInstance(3,cArgs);
      argv[2] = Integer::New(r_req->res);
      argc = 3;
    }
    else
    {
      argv[0] =  Exception::Error(
            String::Concat(String::New("FBblob::EIO_After_Read - "),ERR_MSG_STAT(r_req->status, FBblob)));
      argc = 1;        
    }  
    
    r_req->callback->Call(global, argc, argv);
    
    r_req->callback.Dispose();
    r_req->blob->stop_async();
    r_req->blob->Unref();
    free(r_req);

  }
  
void FBblob::EIO_Read(uv_work_t *req)
  {
    struct rw_request *r_req = (struct rw_request *)(req->data);
    r_req->res = r_req->blob->read(r_req->status,r_req->buffer,(unsigned short) r_req->length);
    return;
  }

Handle<Value> FBblob::Read(const Arguments& args)
  {
    HandleScope scope;
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(args.This());
    
        
    if (args.Length() != 2){
       return ThrowException(Exception::Error(
            String::New("Expecting 2 arguments")));
    }
    
    if (!Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::Error(
                String::New("First argument needs to be a buffer")));
    }
    
    Local<Object> buffer_obj = args[0]->ToObject();
    char *buffer_data = Buffer::Data(buffer_obj);
    size_t buffer_length = Buffer::Length(buffer_obj);
    
    if(!args[1]->IsFunction()) {
      return ThrowException(Exception::Error(
            String::New("Expecting Function as second argument")));
    }
        
    struct rw_request *r_req =
         (struct rw_request *)calloc(1, sizeof(struct rw_request));

    if (!r_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    r_req->blob = blob;
    r_req->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
    r_req->buffer = buffer_data;
    r_req->length = buffer_length;
    r_req->res = 0;

    blob->start_async();

	uv_work_t* req = new uv_work_t();
    req->data = r_req;
    uv_queue_work(uv_default_loop(), req, EIO_Read,  EIO_After_Read);

    
    //uv_ref(uv_default_loop());
    blob->Ref();
    
    return Undefined();
  }
  
Handle<Value>
FBblob::OpenSync(const Arguments& args)
  {
    HandleScope scope;
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(args.This());
    
    ISC_STATUS_ARRAY status;
    if(!blob->open(status)){
       return ThrowException(Exception::Error(
         String::Concat(String::New("In FBblob::_openSync - "),ERR_MSG_STAT(status, FBblob))));
    } 
    
    return Undefined();
  }
  
Handle<Value>
FBblob::CloseSync(const Arguments& args)
  { 
    HandleScope scope;
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(args.This());
    
    ISC_STATUS_ARRAY status;
    status[1] = 0;
    blob->close(status);
    if(status[1]){
       return ThrowException(Exception::Error(
         String::Concat(String::New("In FBblob::_closeSync - "),ERR_MSG_STAT(status, FBblob))));
    } 
    
    return Undefined();
  }
  
Handle<Value>
FBblob::WriteSync(const Arguments& args)
  {
    HandleScope scope;
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(args.This());  
    ISC_STATUS_ARRAY status;     
    
    
    if( (args.Length() > 0) && !Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::Error(
                String::New("First argument needs to be a buffer")));
    }
        
    Local<Object> buffer_obj = args[0]->ToObject();
    char *buf = Buffer::Data(buffer_obj);
    size_t len = Buffer::Length(buffer_obj);
    
    if( (args.Length() > 1) && args[1]->IsInt32() )
    {
      int16_t alen = (int16_t) args[1]->IntegerValue();
      if(alen < len) len = alen;
    }

    if(isc_put_segment(status, &blob->handle, len, buf))
      return ThrowException(Exception::Error(
         String::Concat(String::New("In FBblob::_writeSync - "),ERR_MSG_STAT(status, FBblob))));
         
    return scope.Close(Integer::New(len));     
  }  
  
void FBblob::EIO_After_Write(uv_work_t *req)
  {
    //uv_unref(uv_default_loop());
    HandleScope scope;
    struct rw_request *w_req = (struct rw_request *)(req->data);
	delete req;
    Local<Value> argv[1];
    Local<Object> global = Context::GetCurrent()->Global();
    
    if(!w_req->callback->IsNull()){

	if(w_req->status[1]){
    	    argv[0] =  Exception::Error(
        	String::Concat(String::New("FBblob::EIO_After_Read - "),ERR_MSG_STAT(w_req->status, FBblob)));
	}        
	else
    	    argv[0] = Local<Value>::New(Null());
    	    
        w_req->callback->Call(global, 1, argv);
    };
    
    w_req->callback.Dispose();
//    w_req->blob->Emit();
    w_req->blob->stop_async();
    w_req->blob->Unref();
    free(w_req);

    
  }
  
void FBblob::EIO_Write(uv_work_t *req)
  {
    struct rw_request *w_req = (struct rw_request *)(req->data);
    isc_put_segment(w_req->status, &w_req->blob->handle, w_req->length, w_req->buffer);
    return;
  }
  
  
Handle<Value>
 FBblob::Write(const Arguments& args)
  { 
    HandleScope scope;
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(args.This());
    
    if( (args.Length() > 0) && !Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::Error(
                String::New("First argument needs to be a buffer")));
    }
    
    Local<Object> buffer_obj = args[0]->ToObject();
    char *buf = Buffer::Data(buffer_obj);
    size_t len = Buffer::Length(buffer_obj);
    
    
    struct rw_request *w_req =
         (struct rw_request *)calloc(1, sizeof(struct rw_request));

    if (!w_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate memory.")));
    }
    
    w_req->blob = blob;
    w_req->buffer = buf;

    int cb_arg = 1;    
    if( (args.Length() > 1) && args[1]->IsInt32() )
    {
      int16_t alen = (int16_t) args[1]->IntegerValue();
      if(alen < len) len = alen;
      w_req->length = len;
      cb_arg = 2;
    }
    
    if( (args.Length() > cb_arg) && args[cb_arg]->IsFunction()) {
      w_req->callback = Persistent<Function>::New(Local<Function>::Cast(args[cb_arg]));  
    }
    else w_req->callback = Persistent<Function>::New(Local<Function>::Cast(Local<Value>::New(Null())));

    w_req->res = 0;

    blob->start_async();

	uv_work_t* req = new uv_work_t();
    req->data = w_req;
    uv_queue_work(uv_default_loop(), req, EIO_Write,  EIO_After_Write);

    
    //uv_ref(uv_default_loop());
    blob->Ref();
    
    return Undefined();
       
  }
  
Handle<Value>
FBblob::IsReadGetter(Local<String> property,
                      const AccessorInfo &info)
  {
    HandleScope scope;
    FBblob *blob = ObjectWrap::Unwrap<FBblob>(info.Holder());  
    return scope.Close(Boolean::New(blob->is_read));
  }                        
  
FBblob::FBblob(ISC_QUAD *id, Connection *conn, ISC_STATUS *status): FBEventEmitter () 
  {
    if(id) blob_id = *id; 
    connection = conn;
    is_read = true;
    if((id == 0) && (connection != 0)) 
    {
      handle  = 0;
      //blob_id = 0;
      isc_create_blob2(status, &(connection->db), &(connection->trans), &handle, &blob_id, 0, NULL); 
      is_read = false;
    }
    else handle = NULL;
  }

FBblob::~FBblob()
  {
    if(handle!=0) {
      ISC_STATUS_ARRAY status;
      close(status);
    } 
  }    


bool FBblob::open(ISC_STATUS *status)
  {
    if(isc_open_blob2(status, &(connection->db), &(connection->trans), &handle, &blob_id, 0, NULL)) return false;
    return true; 
  }
  
int FBblob::read(ISC_STATUS *status,char *buf, unsigned short len)
  {
    unsigned short a_read;
    int res = isc_get_segment(status, &handle, &a_read, len, buf);
    if(res == isc_segstr_eof) return 0;
    if(res != isc_segment && status[1] != 0) return -1;
    return (int)a_read;
  }
  
bool FBblob::close(ISC_STATUS *status)
  {
    if( handle == 0 ) return true;
    isc_close_blob(status, &handle);
    handle = 0;
    return true;
  }
