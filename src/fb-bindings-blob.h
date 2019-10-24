/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#ifndef SRC_FB_BINDINGS_BLOB_H_
#define SRC_FB_BINDINGS_BLOB_H_
#define BUILDING_NODE_EXTENSION 1


#include "./fb-bindings.h"
#include <stdlib.h>
#include <node_buffer.h>

#include "./fb-bindings-fbeventemitter.h"
#include "./fb-bindings-transaction.h"



class FBblob : public FBEventEmitter {

public:
 
  static Nan::Persistent<FunctionTemplate> constructor_template;
  
  static void
  Initialize (v8::Local<v8::Object> target);
  
  static bool HasInstance(v8::Local<v8::Value> val);
  void getId(ISC_QUAD* Idp);
  

 protected:
  
  static NAN_METHOD(New);
  static NAN_METHOD(ReadSync);
  static NAN_METHOD(Read);
  static NAN_METHOD(OpenSync);
  static NAN_METHOD(CloseSync);
  static NAN_METHOD(WriteSync);
  static NAN_METHOD(Write);
  
  struct rw_request {
	 Nan::Callback  *callback;
     FBblob *blob;
     char* buffer;
     uint32_t length;
     int res;
     ISC_STATUS_ARRAY status;
  };
  

  static void EIO_After_Read(uv_work_t *req);
  
  static void EIO_Read(uv_work_t *req);

  static void EIO_After_Write(uv_work_t *req);
  
  static void EIO_Write(uv_work_t *req);
  
  static NAN_GETTER(IsReadGetter);
      
  FBblob(ISC_QUAD *id, Transaction *trans, ISC_STATUS *status);
  ~FBblob();
  
  bool open(ISC_STATUS *status);
  int read(ISC_STATUS *status,char *buf, unsigned short len, unsigned short* alen);
  bool close(ISC_STATUS *status);
  
  
  
 private: 
  ISC_QUAD blob_id; 
  Transaction *trans;
  isc_blob_handle handle;
  bool is_read;
  static char err_message[MAX_ERR_MSG_LEN];
  

  
};



#endif
