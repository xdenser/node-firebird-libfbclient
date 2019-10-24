/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#define BUILDING_NODE_EXTENSION 1

#include "./fb-bindings-blob.h"
#include "./fb-bindings-connection.h"

Nan::Persistent<v8::FunctionTemplate> FBblob::constructor_template;
char FBblob::err_message[MAX_ERR_MSG_LEN];

void FBblob::Initialize (v8::Local<v8::Object> target)
  {
  //  HandleScope scope;
    
    Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(FBblob::New);
    constructor_template.Reset(t);

    t->Inherit(Nan::New(FBEventEmitter::constructor_template));
    t->SetClassName(Nan::New("FBblob").ToLocalChecked());

    Local<ObjectTemplate> instance_template =  t->InstanceTemplate();
        
    Nan::SetPrototypeMethod(t, "_readSync", ReadSync);
    Nan::SetPrototypeMethod(t, "_read", Read);
    Nan::SetPrototypeMethod(t, "_openSync", OpenSync);
    Nan::SetPrototypeMethod(t, "_closeSync", CloseSync);
    Nan::SetPrototypeMethod(t, "_writeSync", WriteSync);
    Nan::SetPrototypeMethod(t, "_write", Write);


    instance_template->SetInternalFieldCount(1);
    
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    Nan::SetAccessor(instance_t, Nan::New("inAsyncCall").ToLocalChecked(),InAsyncGetter);
    Nan::SetAccessor(instance_t, Nan::New("isReadable").ToLocalChecked(),IsReadGetter);
    
    Nan::Set(target, Nan::New("FBblob").ToLocalChecked(), Nan::GetFunction(t).ToLocalChecked());

  }

bool FBblob::HasInstance(v8::Local<v8::Value> val)
   { 
       if (!val->IsObject()) return false;
       v8::Local<v8::Object> obj = Nan::To<v8::Object>(val).ToLocalChecked();
/*
  if (obj->GetIndexedPropertiesExternalArrayDataType() == kExternalUnsignedByteArray)
    return true;
*/
       if (Nan::New(constructor_template)->HasInstance(obj))
        return true;

       return false;
   }  
void FBblob::getId(ISC_QUAD* Idp)
  {
    *Idp = blob_id;
  }    
  
NAN_METHOD(FBblob::New)
   {

    Nan::HandleScope scope;

    ISC_QUAD *quad = NULL;
    Connection  *conn = NULL;
    ISC_STATUS_ARRAY status;
    
    if((info.Length() > 0) && !info[0]->IsNull()){
      REQ_EXT_ARG(0, js_quad);
      quad = static_cast<ISC_QUAD *>(js_quad->Value());
    }
    
    if(info.Length() > 1) {
      REQ_EXT_ARG(1, js_connection);
      conn = static_cast<Connection *>(js_connection->Value()); 
    }

    status[1] = 0;
    FBblob *res = new FBblob(quad, conn->def_trans, status);
    if(status[1])  {
        Nan::ThrowError(String::Concat(FB_MAYBE_NEED_ISOLATE
                Nan::New("In FBblob::New - ").ToLocalChecked(),
                ERR_MSG_STAT(status, FBblob)
          ));
    }
    res->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
	
  }
  
NAN_METHOD(FBblob::ReadSync)
  {
	 Nan::HandleScope scope;
    
    FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());
    
    if (!Buffer::HasInstance(info[0])) {
        return Nan::ThrowError("First argument needs to be a buffer");
    }
    
    Local<Object> buffer_obj = Nan::To<Object>(info[0]).ToLocalChecked();
    char *buffer_data = Buffer::Data(buffer_obj);
    size_t buffer_length = Buffer::Length(buffer_obj);
	if (buffer_length > USHRT_MAX) {
		buffer_length = USHRT_MAX;
	}
	unsigned short actual;
    ISC_STATUS_ARRAY status;
    int res = blob->read(status, buffer_data, (unsigned short) buffer_length, &actual);
    if(res==-1) {
       return Nan::ThrowError(String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBblob::New - ").ToLocalChecked(),ERR_MSG_STAT(status, FBblob)));
    }
    
	info.GetReturnValue().Set(Nan::New<Integer>(actual));
  }
  
void FBblob::EIO_After_Read(uv_work_t *req)
  {
   // uv_unref(uv_default_loop());
	Nan::HandleScope scope;
    struct rw_request *r_req = (struct rw_request *)(req->data);
	delete req;
    Local<Value> argv[3];
    int argc;
    
    
    if(r_req->res!=-1)
    {
      argv[0] = Nan::Null();
      argv[1] = Nan::CopyBuffer(r_req->buffer,(size_t) r_req->length).ToLocalChecked();
      argv[2] = (r_req->res==-2) ? Nan::True() : Nan::False();
      argc = 3;
    }
    else
    {
      argv[0] =  Nan::Error(*Nan::Utf8String(
            String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("FBblob::EIO_After_Read - ").ToLocalChecked(),ERR_MSG_STAT(r_req->status, FBblob))));
      argc = 1;        
    }  
    
    Nan::Call(*r_req->callback, argc, argv);
    r_req->blob->stop_async();
    r_req->blob->Unref();
    free(r_req);

  }
  
void FBblob::EIO_Read(uv_work_t *req)
  {
    struct rw_request *r_req = (struct rw_request *)(req->data);
	unsigned short actual = 0;
    r_req->res = r_req->blob->read(r_req->status,r_req->buffer,(unsigned short) r_req->length, &actual);
	r_req->length = actual;
    return;
  }

NAN_METHOD(FBblob::Read)
  {
	Nan::HandleScope scope;
    FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());
    
        
    if (info.Length() != 2){
       return Nan::ThrowError("Expecting 2 arguments");
    }
    
    if (!Buffer::HasInstance(info[0])) {
        return Nan::ThrowError("First argument needs to be a buffer");
    }
    
    Local<Object> buffer_obj = Nan::To<Object>(info[0]).ToLocalChecked();
    char *buffer_data = Buffer::Data(buffer_obj);
    size_t buffer_length = Buffer::Length(buffer_obj);
    
    if(!info[1]->IsFunction()) {
      return Nan::ThrowError("Expecting Function as second argument");
    }
        
    struct rw_request *r_req =
         (struct rw_request *)calloc(1, sizeof(struct rw_request));

    if (!r_req) {
      Nan::LowMemoryNotification();
      return Nan::ThrowError("Could not allocate memory.");
    }
    
    r_req->blob = blob;
    r_req->callback = new Nan::Callback(Local<Function>::Cast(info[1]));
    r_req->buffer = buffer_data;
    r_req->length = (uint32_t) buffer_length;
    r_req->res = 0;

    blob->start_async();

	uv_work_t* req = new uv_work_t();
    req->data = r_req;
    uv_queue_work(uv_default_loop(), req, EIO_Read,  (uv_after_work_cb)EIO_After_Read);

    
    //uv_ref(uv_default_loop());
    blob->Ref();
    
    return;
  }
  
NAN_METHOD(FBblob::OpenSync)
  {
Nan::HandleScope scope;
FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());

ISC_STATUS_ARRAY status;
if (!blob->open(status)) {
	return Nan::ThrowError(
		String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBblob::_openSync - ").ToLocalChecked(), ERR_MSG_STAT(status, FBblob)));
}

return;
  }

  NAN_METHOD(FBblob::CloseSync)
  {
	  Nan::HandleScope scope;
	  FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());

	  ISC_STATUS_ARRAY status;
	  status[1] = 0;
	  blob->close(status);
	  if (status[1]) {
		  return Nan::ThrowError(
			  String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBblob::_closeSync - ").ToLocalChecked(), ERR_MSG_STAT(status, FBblob)));
	  }

	  return;
  }

  NAN_METHOD(FBblob::WriteSync)
  {
	  Nan::HandleScope scope;
	  FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());
	  ISC_STATUS_ARRAY status;


	  if ((info.Length() > 0) && !Buffer::HasInstance(info[0])) {
		  return Nan::ThrowError("First argument needs to be a buffer");
	  }

	  Local<Object> buffer_obj = Nan::To<Object>(info[0]).ToLocalChecked();
	  char *buf = Buffer::Data(buffer_obj);
	  size_t len = Buffer::Length(buffer_obj);

	  if ((info.Length() > 1) && info[1]->IsInt32())
	  {
		  size_t alen = (size_t)Nan::To<int32_t>(info[1]).FromJust();
		  if (alen < len) len = alen;
	  }
	  if (len > USHRT_MAX) {
		  len = USHRT_MAX;
	  }

	  if (isc_put_segment(status, &blob->handle, (unsigned short)len, buf))
		  return Nan::ThrowError(
			  String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBblob::_writeSync - ").ToLocalChecked(), ERR_MSG_STAT(status, FBblob)));

	  info.GetReturnValue().Set(Nan::New<Integer>(uint32_t(len)));
  }

  void FBblob::EIO_After_Write(uv_work_t *req)
  {
	  //uv_unref(uv_default_loop());
	  Nan::HandleScope scope;
	  struct rw_request *w_req = (struct rw_request *)(req->data);
	  delete req;
	  Local<Value> argv[1];

	  if (w_req->callback) {

		  if (w_req->status[1]) {
			  argv[0] = Nan::Error(*Nan::Utf8String(
				  String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("FBblob::EIO_After_Read - ").ToLocalChecked(), ERR_MSG_STAT(w_req->status, FBblob))));
		  }
		  else
			  argv[0] = Nan::Null();

		  Nan::Call(*w_req->callback, 1, argv);
	  };

	  w_req->blob->stop_async();
	  w_req->blob->Unref();
	  free(w_req);


  }

  void FBblob::EIO_Write(uv_work_t *req)
  {
	  struct rw_request *w_req = (struct rw_request *)(req->data);

	  uint32_t remains = w_req->length;
	  char* buf = w_req->buffer;
	  while (remains) {
		  unsigned short len;
		  if (remains > USHRT_MAX) {
			  len = USHRT_MAX / 2;
		  }
		  else {
			  len = remains;
		  }
		  if (isc_put_segment(w_req->status, &w_req->blob->handle, len, buf)) {
		    break;
		  }
		 
		  remains -= len;
		  buf += len;
	  }
  }
  
  
NAN_METHOD(FBblob::Write)
  { 
	Nan::HandleScope scope;
    FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());
    
    if( (info.Length() > 0) && !Buffer::HasInstance(info[0])) {
        return Nan::ThrowError("First argument needs to be a buffer");
    }
    
    Local<Object> buffer_obj = Nan::To<Object>(info[0]).ToLocalChecked();
    char *buf = Buffer::Data(buffer_obj);
    size_t len = Buffer::Length(buffer_obj);
    
    
    struct rw_request *w_req =
         (struct rw_request *)calloc(1, sizeof(struct rw_request));

    if (!w_req) {
      Nan::LowMemoryNotification();
      return Nan::ThrowError("Could not allocate memory.");
    }
    
    w_req->blob = blob;
    w_req->buffer = buf;
	w_req->length = (uint32_t) len;

    int cb_arg = 1;    
    if( (info.Length() > 1) && info[1]->IsInt32() )
    {
      size_t alen = (size_t) Nan::To<int32_t>(info[1]).FromJust();
      if(alen < len) len = alen;
      w_req->length = (uint32_t) len;
      cb_arg = 2;
    }
    
    if( (info.Length() > cb_arg) && info[cb_arg]->IsFunction()) {
      w_req->callback = new Nan::Callback(Local<Function>::Cast(info[cb_arg]));  
    }
    else w_req->callback = NULL;

    w_req->res = 0;

    blob->start_async();

	uv_work_t* req = new uv_work_t();
    req->data = w_req;
    uv_queue_work(uv_default_loop(), req, EIO_Write,  (uv_after_work_cb)EIO_After_Write);

    
    //uv_ref(uv_default_loop());
    blob->Ref();
    
    return;
       
  }
  
NAN_GETTER(FBblob::IsReadGetter)
  {
	Nan::HandleScope scope;
   // FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.Holder());
	FBblob *blob = Nan::ObjectWrap::Unwrap<FBblob>(info.This());
    info.GetReturnValue().Set(Nan::New<Boolean>(blob->is_read));
  }                        
  
FBblob::FBblob(ISC_QUAD *id, Transaction *atrans, ISC_STATUS *status): FBEventEmitter () 
  {
    if(id) blob_id = *id;
	
    trans = atrans;
    is_read = true;
    if((id == 0) && (trans != 0)) 
    {
      handle  = 0;
      blob_id.gds_quad_high = 128;
	  blob_id.gds_quad_low = 0;
      isc_create_blob2(status, &(trans->connection->db), &(trans->trans), &handle, &blob_id, 0, NULL); 
      is_read = false;
    }
    else handle = 0;
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
    if(isc_open_blob2(status, &(trans->connection->db), &(trans->trans), &handle, &blob_id, 0, NULL)) return false;
    return true; 
  }
  
int FBblob::read(ISC_STATUS *status, char *buf, unsigned short len, unsigned short* alen)
  {
    
    ISC_STATUS res = isc_get_segment(status, &handle, alen, len, buf);
	
	if(res == isc_segstr_eof) return -2;
    if(res != isc_segment && status[1] != 0) return -1;
	
    return 0;
  }
  
bool FBblob::close(ISC_STATUS *status)
  {
    if( handle == 0 ) return true;
	if (isc_close_blob(status, &handle)) {
		return false;
	};
    handle = 0;
    return true;
  }
